// JNI adapter called by android/java/Extensions/CRunIniPlusPlus.java.
//
// This deliberately uses only Java/JNI primitives plus the existing C++ bridge. It is not a
// Clickteam RuntimeNative extension and does not emulate or export that private native ABI.

#include "Bridge.hpp"
#include "AndroidRuntime.hpp"
#include "lSDK/UnicodeUtilities.hpp"

#include <jni.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	[[nodiscard]] std::string utf8_from_java(JNIEnv* const env, jstring const value)
	{
		if(!value)
		{
			return {};
		}
		auto const* const chars{env->GetStringChars(value, nullptr)};
		if(!chars)
		{
			return {};
		}
		auto const length{env->GetStringLength(value)};
		std::string result;
		try
		{
			// jchar is a UTF-16 code unit; Android wchar_t is 32 bits, so copy without
			// reinterpreting the JNI buffer as wchar_t or relying on aliasing char16_t.
			std::u16string utf16;
			utf16.reserve(static_cast<std::size_t>(length));
			for(jsize index{}; index < length; ++index)
			{
				utf16.push_back(static_cast<char16_t>(chars[index]));
			}
			result = lSDK::native_from_utf16(std::u16string_view{utf16});
		}
		catch(...)
		{
			env->ReleaseStringChars(value, chars);
			throw;
		}
		env->ReleaseStringChars(value, chars);
		return result;
	}

	[[nodiscard]] jstring java_from_utf8(JNIEnv* const env, std::string_view const value)
	{
		auto const utf16{lSDK::utf16_from_native(value)};
		return env->NewString(reinterpret_cast<jchar const*>(utf16.data()), static_cast<jsize>(utf16.size()));
	}

	[[nodiscard]] std::vector<ipp_android::bridge::InputValue> read_values(
		JNIEnv* const env, jobjectArray const values)
	{
		std::vector<ipp_android::bridge::InputValue> result;
		if(!values)
		{
			return result;
		}

		jclass const string_class{env->FindClass("java/lang/String")};
		jclass const number_class{env->FindClass("java/lang/Number")};
		jclass const integer_class{env->FindClass("java/lang/Integer")};
		if(!string_class || !number_class || !integer_class)
		{
			return result;
		}
		jmethodID const double_value{env->GetMethodID(number_class, "doubleValue", "()D")};
		jmethodID const int_value{env->GetMethodID(number_class, "intValue", "()I")};
		if(!double_value || !int_value)
		{
			return result;
		}

		jsize const count{env->GetArrayLength(values)};
		result.reserve(static_cast<std::size_t>(count));
		for(jsize index{}; index < count; ++index)
		{
			jobject const item{env->GetObjectArrayElement(values, index)};
			if(!item)
			{
				result.emplace_back(ipp_android::bridge::InputValue::object());
				continue;
			}
			if(env->IsInstanceOf(item, string_class))
			{
				result.emplace_back(ipp_android::bridge::InputValue::from_string(
					utf8_from_java(env, static_cast<jstring>(item))));
			}
			else if(env->IsInstanceOf(item, number_class))
			{
				if(env->IsInstanceOf(item, integer_class))
				{
					result.emplace_back(ipp_android::bridge::InputValue::from_integer(
						env->CallIntMethod(item, int_value)));
				}
				else
				{
					result.emplace_back(ipp_android::bridge::InputValue::from_number(
						env->CallDoubleMethod(item, double_value)));
				}
			}
			else
			{
				result.emplace_back(ipp_android::bridge::InputValue::object());
			}
			env->DeleteLocalRef(item);
			if(env->ExceptionCheck())
			{
				return {};
			}
		}
		env->DeleteLocalRef(integer_class);
		env->DeleteLocalRef(number_class);
		env->DeleteLocalRef(string_class);
		return result;
	}

	[[nodiscard]] ipp_android::bridge::Extension* from_handle(jlong const handle) noexcept
	{
		return reinterpret_cast<ipp_android::bridge::Extension*>(static_cast<std::intptr_t>(handle));
	}

	[[nodiscard]] jobject make_expression_result(JNIEnv* const env,
	                                             ipp_android::bridge::ExpressionResult const& result)
	{
		jclass const value_class{env->FindClass("Expressions/CValue")};
		if(!value_class)
		{
			return nullptr;
		}
		jobject value{};
		switch(result.kind)
		{
			case ipp_android::bridge::ExpressionResult::Kind::Integer:
			{
				jmethodID const constructor{env->GetMethodID(value_class, "<init>", "(I)V")};
				if(constructor)
				{
					value = env->NewObject(value_class, constructor, static_cast<jint>(result.integer));
				}
				break;
			}
			case ipp_android::bridge::ExpressionResult::Kind::Float:
			{
				jmethodID const constructor{env->GetMethodID(value_class, "<init>", "(D)V")};
				if(constructor)
				{
					value = env->NewObject(value_class, constructor, static_cast<jdouble>(result.floating));
				}
				break;
			}
			case ipp_android::bridge::ExpressionResult::Kind::String:
			{
				jmethodID const constructor{env->GetMethodID(value_class, "<init>", "(Ljava/lang/String;)V")};
				jstring const text{java_from_utf8(env, result.text)};
				if(constructor && text)
				{
					value = env->NewObject(value_class, constructor, text);
				}
				if(text)
				{
					env->DeleteLocalRef(text);
				}
				break;
			}
		}
		env->DeleteLocalRef(value_class);
		return value;
	}

	void log_jni_exception(char const* const where) noexcept
	{
		try
		{
			::ipp_android::log(std::string{"Ini++: JNI bridge failed in "} + (where ? where : "unknown call"));
		}
		catch(...)
		{
			// Logging must not throw across a JNI boundary.
		}
	}
}

extern "C"
{
	JNIEXPORT jint JNICALL Java_Extensions_CRunIniPlusPlus_nativeGetNumberOfConditions(JNIEnv*, jclass)
	{
		return ipp_android::bridge::condition_count();
	}

	JNIEXPORT jlong JNICALL Java_Extensions_CRunIniPlusPlus_nativeCreate(
		JNIEnv* const env, jclass, jbyteArray const edit_data, jint const version, jstring const files_directory)
	{
		try
		{
			std::vector<std::byte> bytes;
			if(edit_data)
			{
				jsize const count{env->GetArrayLength(edit_data)};
				bytes.resize(static_cast<std::size_t>(count));
				if(count > 0)
				{
					env->GetByteArrayRegion(edit_data, 0, count, reinterpret_cast<jbyte*>(bytes.data()));
				}
				if(env->ExceptionCheck())
				{
					return 0;
				}
			}
			auto const root{utf8_from_java(env, files_directory)};
			auto* const extension{ipp_android::bridge::create(
				std::span<std::byte const>{bytes}, static_cast<int>(version), std::filesystem::path{root})};
			return static_cast<jlong>(reinterpret_cast<std::intptr_t>(extension));
		}
		catch(std::exception const&)
		{
			log_jni_exception("nativeCreate");
			return 0;
		}
		catch(...)
		{
			log_jni_exception("nativeCreate");
			return 0;
		}
	}

	JNIEXPORT void JNICALL Java_Extensions_CRunIniPlusPlus_nativeDestroy(
		JNIEnv*, jclass, jlong const handle, jboolean const fast)
	{
		ipp_android::bridge::destroy(from_handle(handle), fast == JNI_TRUE);
	}

	JNIEXPORT jint JNICALL Java_Extensions_CRunIniPlusPlus_nativeHandle(JNIEnv*, jclass, jlong const handle)
	{
		return ipp_android::bridge::handle(from_handle(handle));
	}

	JNIEXPORT void JNICALL Java_Extensions_CRunIniPlusPlus_nativeAction(
		JNIEnv* const env, jclass, jlong const handle, jint const id, jobjectArray const parameters)
	{
		try
		{
			auto const values{read_values(env, parameters)};
			ipp_android::bridge::action(from_handle(handle), static_cast<int>(id), values);
		}
		catch(...)
		{
			log_jni_exception("nativeAction");
		}
	}

	JNIEXPORT jboolean JNICALL Java_Extensions_CRunIniPlusPlus_nativeCondition(
		JNIEnv* const env, jclass, jlong const handle, jint const id, jobjectArray const parameters)
	{
		try
		{
			auto const values{read_values(env, parameters)};
			return ipp_android::bridge::condition(from_handle(handle), static_cast<int>(id), values) ? JNI_TRUE : JNI_FALSE;
		}
		catch(...)
		{
			log_jni_exception("nativeCondition");
			return JNI_FALSE;
		}
	}

	JNIEXPORT jobject JNICALL Java_Extensions_CRunIniPlusPlus_nativeExpression(
		JNIEnv* const env, jclass, jlong const handle, jint const id, jobjectArray const parameters)
	{
		try
		{
			auto const values{read_values(env, parameters)};
			auto const result{ipp_android::bridge::expression(from_handle(handle), static_cast<int>(id), values)};
			return make_expression_result(env, result);
		}
		catch(...)
		{
			log_jni_exception("nativeExpression");
			return nullptr;
		}
	}
}
