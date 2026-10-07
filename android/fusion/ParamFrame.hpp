#ifndef IniPlusPlus_AndroidParamFrame_HeaderPlusPlus
#define IniPlusPlus_AndroidParamFrame_HeaderPlusPlus

/*
 * Ini++ Android runtime: the ACE parameter frame.
 *
 * On Windows the Fusion runtime hands an extension:
 *   * the first two parameters as `param0` / `param1` (raw 32 bit values), and
 *   * a parameter cursor that `CNC_GetIntParameter`, `CNC_GetStringParameter`,
 *     `CNC_GetFloatParameter` and `CNC_GetFirst/NextExpressionParameter` read from, in the order
 *     the ACE reads them.
 *
 * The Android native extension API offers the same thing with a different spelling: the typed
 * sequential accessors act_/cnd_/exp_getParam{Expression,ExpString,ExpFloat}, each of which
 * evaluates and consumes the next parameter of the ACE currently being run.  This frame forwards
 * the shared ACE bodies' `CNC_*` reads to those accessors, so on Android the parameters are read
 * exactly when and in the order the Windows implementation reads them - including ACEs that skip a
 * parameter depending on which variant of an action they are (the "current group" actions).
 *
 * The frame also owns
 *   * the string pool that keeps strings read from the runtime alive for the duration of the ACE
 *     (the runtime's strings are returned to it with freeString as soon as they are copied), and
 *   * the return buffer used by ACEs.cpp's temp_string()/callRunTimeFunction(GETSTRINGSPACE_EX).
 *
 * Types and coercions match the Windows runtime:
 *   * reading a string parameter as a number yields 0
 *   * reading a number parameter as a string yields an empty string
 *   * HOF_STRING / HOF_FLOAT on run_data->rHo.hoFlags select what an expression returns
 */

#include <cstddef>
#include <bit>
#include <cstdint>
#include <deque>
#include <vector>
#include <string>
#include <vector>

namespace ipp_android
{
	// A zeroed block handed to ACEs that ask for an object or a custom parameter: the Android
	// extension API cannot deliver either, so ACEs see all-zero values (documented defaults) instead
	// of dereferencing a null pointer.  See docs/COMPATIBILITY.md.
	[[nodiscard]] void* empty_object() noexcept;

	// Implemented by the runtime glue (android/jni/IniPlusPlusExtension.cc) for the ACE being run.
	struct ParamSource
	{
		void* context{};
		std::int32_t (*integer)(void* context){};      // next parameter as an integer
		float (*floating)(void* context){};           // next parameter as a float
		std::string (*string)(void* context){};       // next parameter as UTF-8 text
		std::intptr_t (*expression)(void* context, std::int32_t type){}; // TYPE_INT/STRING/FLOAT
	};

	struct Param final
	{
		enum class Kind : std::uint8_t { Number, String, Object };
		Kind kind{Kind::Number};
		std::int32_t int_value{};
		std::int32_t float_bits{};
		std::string text;
		void* object{};
	};

	class ParamFrame final
	{
	public:
		// Conditions receive their parameters as the two raw slots param0/param1, so the runtime
		// glue reads the declared parameter list into `params` before the ACE runs and slot()
		// hands the values over.  Actions and expressions pull their parameters lazily through the
		// ParamSource below, exactly when the Windows implementation would read them.
		std::vector<Param> params;
		mutable std::size_t position{}; // cursor into `params` for CNC_* reads
		std::int32_t ace_number{}; // becomes run_data->rHo.hoEventNumber
		std::int32_t flags{};      // becomes run_data->rHo.hoFlags (HOF_*)

		void set_source(ParamSource const* const new_source) noexcept { source = new_source; }
		void reset() noexcept
		{
			params.clear();
			position = 0;
			pool.clear();
			ace_number = 0;
			flags = 0;
			return_buffer.clear();
		}

		// ---- CNC_* equivalents ---------------------------------------------------------------
		// A parameter that is not available on Android (an object reference or a custom parameter
		// payload) is the zeroed block: ACEs see all-zero values (documented defaults) instead of
		// dereferencing a null pointer.  See docs/COMPATIBILITY.md.
		void* cnc_get_object() const noexcept
		{
			auto const* const param{peek()};
			return (param && param->kind == Param::Kind::Object && param->object) ? param->object : empty_object();
		}
		std::int32_t cnc_get_int() const noexcept
		{
			auto const* const param{peek()};
			if(param)
			{
				return param->kind == Param::Kind::Number ? param->int_value : 0;
			}
			return source && source->integer ? source->integer(source->context) : 0;
		}
		// The Windows runtime returns a float parameter as its 32 bit pattern cast to an integer
		// (the ACE bodies recover it with reinterpret_cast), so this must not return a float.
		std::int32_t cnc_get_float() const noexcept
		{
			auto const* const param{peek()};
			if(param)
			{
				return param->kind == Param::Kind::Number ? param->float_bits : 0;
			}
			return source && source->floating ? std::bit_cast<std::int32_t>(source->floating(source->context)) : 0;
		}
		char const* cnc_get_string() const noexcept
		{
			if(auto const* const param{peek()})
			{
				return param->kind == Param::Kind::String ? param->text.c_str() : "";
			}
			if(!source || !source->string)
			{
				return "";
			}
			try
			{
				// Keep the text alive until the runtime call returns; the ACE bodies hold on to
				// the pointer for as long as they need the parameter.
				return pool.emplace_back(source->string(source->context)).c_str();
			}
			catch(...)
			{
				return "";
			}
		}
		// Expression parameters: the ACE asks for a type and the runtime converts, exactly like the
		// Windows runtime's RFUNCTION_GETPARAM (TYPE_INT = 0, TYPE_STRING = 1, TYPE_FLOAT = 2).
		std::intptr_t cnc_get_expression(std::int32_t const type) const noexcept
		{
			if(auto const* const param{peek()})
			{
				switch(type)
				{
					case 0: return param->kind == Param::Kind::Number ? param->int_value : 0;
					case 1: return param->kind == Param::Kind::String ? reinterpret_cast<std::intptr_t>(param->text.c_str()) : 0;
					case 2: return param->kind == Param::Kind::Number ? param->float_bits : 0;
					default: return 0;
				}
			}
			return source && source->expression ? source->expression(source->context, type) : 0;
		}

		// The two raw parameters the generated dispatch passes to the ACE.  Object references and
		// custom parameter payloads are not available through the Android extension API, so the
		// zeroed block is handed over (documented defaults).
		[[nodiscard]] std::intptr_t slot(std::size_t const i) const noexcept
		{
			if(i >= params.size())
			{
				return reinterpret_cast<std::intptr_t>(empty_object());
			}
			auto const& param{params[i]};
			switch(param.kind)
			{
				case Param::Kind::String: return reinterpret_cast<std::intptr_t>(param.text.c_str());
				case Param::Kind::Object: return reinterpret_cast<std::intptr_t>(param.object ? param.object : empty_object());
				case Param::Kind::Number: return param.int_value;
			}
			return 0;
		}

		// temp_string()/callRunTimeFunction(RFUNCTION_GETSTRINGSPACE_EX)'s backing store; the
		// pointer stays valid until the frame is reset at the start of the next ACE.
		char* allocate_return_string(std::size_t const bytes)
		{
			return_buffer.assign(bytes + sizeof(char), '\0');
			return return_buffer.data();
		}

	private:
		[[nodiscard]] Param const* peek() const noexcept
		{
			if(position >= params.size())
			{
				return nullptr;
			}
			++position;
			return &params[position - 1];
		}

		// mutable: the CNC_* accessors are const (they are called through const RunData helpers),
		// but they do advance the runtime's parameter cursor and intern strings.
		mutable std::deque<std::string> pool;
		std::vector<char> return_buffer;
		ParamSource const* source{};
	};
}

#endif
