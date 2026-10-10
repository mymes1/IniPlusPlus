#ifndef IniPlusPlus_AndroidParamFrame_HeaderPlusPlus
#define IniPlusPlus_AndroidParamFrame_HeaderPlusPlus

/*
 * Android's per-ACE parameter frame for the shared C++ bodies.
 *
 * The Java CRunExtension adapter evaluates each argument with the public CActExtension,
 * CCndExtension or CValue API in the generated Menus.cpp order. JniBridge.cpp converts these
 * values to InputValue and Bridge.cpp builds this frame, so the C++ ACE code can keep using the
 * Windows-side CNC_* interface and the same ACE identifiers/order.
 *
 * This is a source-level adapter, not a Clickteam native ABI. Object references and Ini++ custom
 * parameter payloads currently enter as safe placeholders; those Android differences are listed
 * in docs/COMPATIBILITY.md. The frame owns transient strings and expression return storage for each
 * synchronous JNI call.
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
	// Safe placeholder used for object/custom parameters that this Java/JNI adapter does not yet
	// marshal. The Java API exposes these values, but the bridge currently preserves the parameter
	// slot with zero defaults instead of fabricating a Fusion C++ object. See docs/COMPATIBILITY.md.
	[[nodiscard]] void* empty_object() noexcept;

	// Per-call callbacks used by the bridge to consume generated expression arguments in order.
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
		// The generated dispatcher reads action/condition arguments into these slots in Menus.cpp
		// order. Expressions consume their already-evaluated arguments lazily through ParamSource,
		// matching the shared C++ body's CNC_* call order.
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
		// Object/custom values not marshalled by the current Java/JNI adapter receive a safe zeroed
		// placeholder. ACEs retain their positions and never dereference a null pointer.
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

		// The two raw parameters the generated dispatch passes to the ACE; un-marshalled object or
		// custom parameters retain their index through a safe zeroed placeholder.
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
