// Java/JNI-independent core of the Android extension adapter.
//
// Fusion's public Java CRunExtension API is the only Android runtime interface used here. The
// JNI-specific methods in JniBridge.cpp translate Java values to InputValue and call this bridge;
// this file owns the existing C++ RunData and dispatches the shared ACE bodies in ACEs.cpp.

#include "FusionAPI.hpp"
#include "EditData.hpp"
#include "RunData.hpp"
#include "Bridge.hpp"
#include "AndroidRuntime.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AceDeclarations.inc"

std::int16_t FUSION_API CreateRunObject(RunData* const, SerializedEditData const*, createObjectInfo*) noexcept;
std::int16_t FUSION_API DestroyRunObject(RunData* const, std::int32_t) noexcept;
std::int16_t FUSION_API HandleRunObject(RunData* const) noexcept;

namespace ipp_android
{
	using bridge::InputValue;
	using bridge::ValueKind;
	using bridge::ExpressionResult;

	namespace
	{
		constexpr std::size_t max_edit_data_size{64u * 1024u * 1024u};

		[[nodiscard]] std::int32_t integer_value(InputValue const* const value) noexcept
		{
			if(!value)
			{
				return 0;
			}
			switch(value->kind)
			{
				case ValueKind::Integer:
					return value->integer;
				case ValueKind::Number:
					if(!std::isfinite(value->number))
					{
					return 0;
				}
				if(value->number >= static_cast<double>(std::numeric_limits<std::int32_t>::max()))
				{
					return std::numeric_limits<std::int32_t>::max();
				}
				if(value->number <= static_cast<double>(std::numeric_limits<std::int32_t>::min()))
				{
					return std::numeric_limits<std::int32_t>::min();
				}
				return static_cast<std::int32_t>(value->number);
				case ValueKind::String:
				case ValueKind::Object:
					return 0;
			}
			return 0;
		}

		[[nodiscard]] float floating_value(InputValue const* const value) noexcept
		{
			if(!value)
			{
				return 0.0f;
			}
			switch(value->kind)
			{
				case ValueKind::Integer:
					return static_cast<float>(value->integer);
				case ValueKind::Number:
					return std::isfinite(value->number) ? static_cast<float>(value->number) : 0.0f;
				case ValueKind::String:
				case ValueKind::Object:
					return 0.0f;
			}
			return 0.0f;
		}

		[[nodiscard]] std::string string_value(InputValue const* const value)
		{
			return value && value->kind == ValueKind::String ? value->text : std::string{};
		}

		[[nodiscard]] std::vector<std::byte> normalize_edit_data(
			std::span<std::byte const> edit_data, std::int32_t version)
		{
			if(edit_data.size() > max_edit_data_size - sizeof(extHeader))
			{
				throw std::length_error("Ini++ edit data exceeds 64 MiB");
			}

			// CExtension.init() passes CBinaryFile.data, which may be either the extension payload
			// or a full extHeader-prefixed blob depending on the exporter loader. Accept both shapes;
			// the exact Fusion 295.10 pointer convention still needs the compatible exporter test.
			if(edit_data.size() >= sizeof(extHeader))
			{
				extHeader header{};
				std::memcpy(&header, edit_data.data(), sizeof(header));
				if((header.extVersion == SerializedEditData::INIPP_V1_ANSI
				 || header.extVersion == SerializedEditData::INIPP_V2_UNICODE)
				&& header.extSize >= sizeof(extHeader)
				&& header.extSize <= edit_data.size())
				{
					return {edit_data.begin(), edit_data.begin() + header.extSize};
				}
			}

			if(version != SerializedEditData::INIPP_V1_ANSI
			&& version != SerializedEditData::INIPP_V2_UNICODE)
			{
				::ipp_android::log("Ini++: Android loader supplied an unknown edit-data version; using Unicode v2 defaults");
				version = SerializedEditData::INIPP_V2_UNICODE;
			}

			std::vector<std::byte> blob(sizeof(extHeader) + edit_data.size());
			extHeader header{};
			header.extSize = static_cast<std::uint32_t>(blob.size());
			header.extMaxSize = header.extSize;
			header.extVersion = static_cast<std::uint32_t>(version);
			std::memcpy(blob.data(), &header, sizeof(header));
			std::copy(edit_data.begin(), edit_data.end(), blob.begin() + sizeof(extHeader));
			return blob;
		}
	}

	class AceParamReader final
	{
	public:
		explicit AceParamReader(std::span<InputValue const> values) noexcept
		: values{values}
		{
			source.context = this;
			source.integer = [](void* context) -> std::int32_t
			{
				return static_cast<AceParamReader*>(context)->next_integer();
			};
			source.floating = [](void* context) -> float
			{
				return static_cast<AceParamReader*>(context)->next_float();
			};
			source.string = [](void* context) -> std::string
			{
				return static_cast<AceParamReader*>(context)->next_string();
			};
			source.expression = [](void* context, std::int32_t type) -> std::intptr_t
			{
				return static_cast<AceParamReader*>(context)->next_expression(type);
			};
		}

		[[nodiscard]] ParamSource const* params() const noexcept { return &source; }

		// Actions and conditions receive raw param0/param1 slots in the shared ACE bodies. The
		// generated dispatcher stores the Java-evaluated arguments here in declared order first.
		void add_number(std::vector<Param>& params)
		{
			auto const* const value{next()};
			Param param{};
			param.kind = Param::Kind::Number;
			auto const number{floating_value(value)};
			param.int_value = integer_value(value);
			param.float_bits = std::bit_cast<std::int32_t>(number);
			params.emplace_back(std::move(param));
		}

		void add_position(std::vector<Param>& params)
		{
			auto const* const value{next()};
			Param param{};
			param.kind = Param::Kind::Number;
			param.int_value = integer_value(value);
			param.float_bits = std::bit_cast<std::int32_t>(static_cast<float>(param.int_value));
			params.emplace_back(std::move(param));
		}

		void add_string(std::vector<Param>& params)
		{
			Param param{};
			param.kind = Param::Kind::String;
			param.text = string_value(next());
			params.emplace_back(std::move(param));
		}

		void add_object(std::vector<Param>& params)
		{
			// The Java adapter deliberately maps object references and Fusion custom-parameter data
			// to null. The next value is still consumed so every later parameter keeps its ACE slot.
			static_cast<void>(next());
			Param param{};
			param.kind = Param::Kind::Object;
			param.object = nullptr;
			params.emplace_back(std::move(param));
		}

	private:
		[[nodiscard]] InputValue const* next() noexcept
		{
			if(position >= values.size())
			{
				return nullptr;
			}
			return &values[position++];
		}

		[[nodiscard]] std::int32_t next_integer() noexcept { return integer_value(next()); }
		[[nodiscard]] float next_float() noexcept { return floating_value(next()); }
		[[nodiscard]] std::string next_string() { return string_value(next()); }

		[[nodiscard]] std::intptr_t next_expression(std::int32_t const type)
		{
			auto const* const value{next()};
			switch(type)
			{
				case TYPE_INT:
					return static_cast<std::intptr_t>(integer_value(value));
				case TYPE_STRING:
					return reinterpret_cast<std::intptr_t>(strings.emplace_back(string_value(value)).c_str());
				case TYPE_FLOAT:
					return static_cast<std::intptr_t>(std::bit_cast<std::int32_t>(floating_value(value)));
				default:
					return 0;
			}
		}

		std::span<InputValue const> values;
		std::size_t position{};
		std::deque<std::string> strings;
		ParamSource source{};
	};

#include "AceDispatch.inc"
}

namespace ipp_android::bridge
{
	struct Extension final
	{
		alignas(RunData) std::byte storage[sizeof(RunData)]{};
		RunData* run_data{};
		CRunApp app{};
		RunHeader run_header{};
		mv runtime_mv{};
	};

	InputValue InputValue::from_number(double value)
	{
		InputValue result{};
		result.kind = ValueKind::Number;
		result.number = value;
		return result;
	}

	InputValue InputValue::from_integer(std::int32_t value)
	{
		InputValue result{};
		result.kind = ValueKind::Integer;
		result.integer = value;
		return result;
	}

	InputValue InputValue::from_string(std::string value)
	{
		InputValue result{};
		result.kind = ValueKind::String;
		result.text = std::move(value);
		return result;
	}

	InputValue InputValue::object()
	{
		return {};
	}

	int condition_count() noexcept
	{
		return number_of_conditions;
	}

	Extension* create(std::span<std::byte const> const edit_data, int const edit_data_version,
	                  std::filesystem::path files_directory) noexcept
	{
		try
		{
			if(files_directory.empty())
			{
				::ipp_android::log("Ini++: Java bridge did not provide Context.getFilesDir(); object creation refused");
				return nullptr;
			}
			::ipp_android::set_data_dir(std::move(files_directory));
			auto normalized{normalize_edit_data(edit_data, edit_data_version)};
			auto* const extension{new (std::nothrow) Extension{}};
			if(!extension)
			{
				::ipp_android::log("Ini++: out of memory while creating the Android extension object");
				return nullptr;
			}

			std::memset(extension->storage, 0, sizeof(extension->storage));
			extension->run_data = reinterpret_cast<RunData*>(extension->storage);
			extension->app.m_dwCodePage = 1252;
			extension->run_header.rhApp = &extension->app;
			extension->run_header.rh4.rh4Mv = &extension->runtime_mv;
			extension->run_data->rHo.hoAdRunHeader = &extension->run_header;

			auto const* const serialized{reinterpret_cast<SerializedEditData const*>(normalized.data())};
			if(CreateRunObject(extension->run_data, serialized, nullptr) != FUSION_FINISH_RUNTIME_STRUCTURE_CONSTRUCTION_SUCCESS)
			{
				::ipp_android::log("Ini++: shared CreateRunObject() rejected the Android edit data");
				delete extension;
				return nullptr;
			}
			return extension;
		}
		catch(std::exception const& error)
		{
			::ipp_android::log(std::string{"Ini++: Android object creation failed: "} + error.what());
		}
		catch(...)
		{
			::ipp_android::log("Ini++: Android object creation failed with an unknown error");
		}
		return nullptr;
	}

	void destroy(Extension* const extension, bool const fast) noexcept
	{
		if(!extension)
		{
			return;
		}
		if(extension->run_data)
		{
			static_cast<void>(DestroyRunObject(extension->run_data, fast ? 1 : 0));
			extension->run_data = nullptr;
		}
		delete extension;
	}

	int handle(Extension* const extension) noexcept
	{
		if(!extension || !extension->run_data)
		{
			return FUSION_ON_TICK_DONT_DRAW;
		}
		return HandleRunObject(extension->run_data);
	}

	void action(Extension* const extension, int const id,
	            std::span<InputValue const> const parameters) noexcept
	{
		if(!extension || !extension->run_data)
		{
			return;
		}
		try
		{
			AceParamReader reader{parameters};
			auto& frame{current_frame()};
			frame.set_source(nullptr);
			static_cast<void>(dispatch_action(id, extension->run_data, frame, reader));
		}
		catch(...)
		{
			log("Ini++: exception while executing an Android action");
		}
	}

	bool condition(Extension* const extension, int const id,
	               std::span<InputValue const> const parameters) noexcept
	{
		if(!extension || !extension->run_data)
		{
			return false;
		}
		try
		{
			AceParamReader reader{parameters};
			auto& frame{current_frame()};
			frame.set_source(nullptr);
			return dispatch_condition(id, extension->run_data, frame, reader) != 0;
		}
		catch(...)
		{
			log("Ini++: exception while executing an Android condition");
			return false;
		}
	}

	ExpressionResult expression(Extension* const extension, int const id,
	                            std::span<InputValue const> const parameters) noexcept
	{
		ExpressionResult result{};
		if(!extension || !extension->run_data)
		{
			return result;
		}
		auto& frame{current_frame()};
		frame.set_source(nullptr);
		try
		{
			AceParamReader reader{parameters};
			frame.set_source(reader.params());
			extension->run_data->rHo.hoFlags = 0;
			auto const raw{dispatch_expression(id, extension->run_data, frame, reader)};
			frame.set_source(nullptr);

			if((extension->run_data->rHo.hoFlags & HOF_STRING) != 0)
			{
				result.kind = ExpressionResult::Kind::String;
				auto const* const text{reinterpret_cast<char const*>(static_cast<std::intptr_t>(raw))};
				result.text = text ? text : "";
			}
			else if((extension->run_data->rHo.hoFlags & HOF_FLOAT) != 0)
			{
				result.kind = ExpressionResult::Kind::Float;
				result.floating = std::bit_cast<float>(static_cast<std::int32_t>(raw));
			}
			else
			{
				result.kind = ExpressionResult::Kind::Integer;
				result.integer = static_cast<std::int32_t>(raw);
			}
		}
		catch(...)
		{
			frame.set_source(nullptr);
			log("Ini++: exception while executing an Android expression");
		}
		return result;
	}
}
