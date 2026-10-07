// Ini++ - Fusion 2.5 Android runtime extension (official Android exporter native extension ABI).
//
// The Android exporter loads extension natives from the built application's `assets/mmf/<abi>`
// folder: every file there whose name starts with "CRun" is registered as a native extension and
// handed to the runtime's native loader (Runtime/MMFRuntime.java + Runtime/Native.java in the
// Android runtime source; see github.com/ClickteamLLC/android and the Android SDK's
// native/CRunTemplate).  For an MFA that uses `INI++.mfx`, the extension name as parsed by the
// runtime is "INI++" (CExtLoad: filename minus its last dot-segment, '-' and ' ' replaced by '_'),
// so the runtime looks for `CRunINI++.so` and, through its native loader, for these entry points:
//
//     CRunINI++_extInit                void  ()
//     CRunINI++_createRunObject        void* (EDITDATA* edPtr, void* ext, RuntimeFunctions* fn)
//     CRunINI++_getNumberOfConditions  int   ()
//     CRunINI++_destroyRunObject       void  (Extension*)
//     CRunINI++_handleRunObject        int   (Extension*)
//     CRunINI++_action                 void  (Extension*, int num, void* act)
//     CRunINI++_condition              bool  (Extension*, int num, void* cnd)
//     CRunINI++_expression             void  (Extension*, int num, void* exp)
//
// (`CRunINI++` is not a valid C identifier, so those symbols are exported through __asm__ labels;
// the same functions are also exported under the plain identifier CRunIniPlusPlus for MFX files
// that were renamed.  Loader-side reference: the SDK's native/base/NativeExtension.cc, whose
// ExportB/ExportA token pasting produces exactly these symbol names for an extension whose module
// name is a valid identifier.)
//
// The ACE bodies themselves are shared with the Windows MFX: Runtime.cpp and ACEs.cpp, unchanged
// except for the FUSION_ANDROID_RUNTIME guards, dispatch through android/runtime/AceDispatch.inc
// (generated from Menus.cpp by tools/gen_android_aces.py) and read their parameters from the
// current ParamFrame through the same RuntimeFunctions accessors the official template uses.
// Everything platform-specific is behind android/fusion/ (Windows API shim) and
// android/runtime/AndroidRuntime.cpp (file access, logging, edit data).
//
// Build: tools/build_android.sh (ndk-build).  The one proprietary dependency is the SDK's
// <RuntimeNative.h> - see docs/BUILD.md.  android/tests/mock-sdk/RuntimeNative.h is a host-test
// stand-in and must never be shipped.

#if defined(__has_include)
#	if !__has_include(<RuntimeNative.h>)
#		error "RuntimeNative.h was not found.  Point the build at the Clickteam Android SDK (tools/build_android.sh IPPP_ANDROID_SDK_DIR=...), or - for host tests only - at android/tests/mock-sdk.  See docs/BUILD.md."
#	endif
#endif
#include <RuntimeNative.h>

#include "FusionAPI.hpp"
#include "lSDK.hpp"
#include "EditData.hpp"
#include "RunData.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <utility>
#include <vector>

// ACE ids and the shared ACE bodies (generated; see tools/gen_android_aces.py)
#include "AceDeclarations.inc"

// -------------------------------------------------------------------------------------------------
// The shared, platform neutral entry points (Runtime.cpp)
// -------------------------------------------------------------------------------------------------
std::uint16_t FUSION_API GetRunObjectDataSize(RunHeader* const, SerializedEditData const* const) noexcept;
std::int16_t  FUSION_API CreateRunObject(RunData* const, SerializedEditData const* const, createObjectInfo* const) noexcept;
std::int16_t  FUSION_API DestroyRunObject(RunData* const, std::int32_t const) noexcept;
std::int16_t  FUSION_API HandleRunObject(RunData* const) noexcept;

namespace ipp_android
{
	class AceParamReader;

	// The generated dispatch tables at the bottom of this file (see tools/gen_android_aces.py)
	[[nodiscard]] ::fusion::action_return_t dispatch_action(std::int32_t num, ::RunData* run_data, ParamFrame& frame, AceParamReader& reader) noexcept;
	[[nodiscard]] ::fusion::condition_return_t dispatch_condition(std::int32_t num, ::RunData* run_data, ParamFrame& frame, AceParamReader& reader) noexcept;
	[[nodiscard]] ::fusion::expression_return_t dispatch_expression(std::int32_t num, ::RunData* run_data, ParamFrame& frame, AceParamReader& reader) noexcept;

	// ---------------------------------------------------------------------------------------------
	// Parameter reading.  The runtime hands out one value per call, in the order the ACE was
	// authored (Menus.cpp); the runtime owns the string memory and it is returned as soon as the
	// value has been copied out.
	// ---------------------------------------------------------------------------------------------
	enum class AceKind : int { action_id, condition_id, expression_id };

	class AceParamReader final
	{
	public:
		AceParamReader(RuntimeFunctions& runtime, void* const ext, AceKind const kind) noexcept
		: rt{runtime}, ext{ext}, kind{kind}
		{
			source.context = this;
			source.integer = [](void* const c) -> std::int32_t { return static_cast<std::int32_t>(static_cast<AceParamReader*>(c)->next_float()); };
			source.floating = [](void* const c) -> float { return static_cast<AceParamReader*>(c)->next_float(); };
			source.string = [](void* const c) -> std::string { return static_cast<AceParamReader*>(c)->next_string(); };
			source.expression = [](void* const c, std::int32_t const type) -> std::intptr_t
			{
				return static_cast<AceParamReader*>(c)->next_expression(type);
			};
		}

		[[nodiscard]] ParamSource const* params() const noexcept { return &source; }

		// Eager reads, used by the generated dispatch for condition parameters (conditions receive
		// their parameters as the two raw slots, not through a cursor).
		void add_number(std::vector<Param>& params)
		{
			Param param{};
			param.kind = Param::Kind::Number;
			auto const value{next_float()};
			param.float_bits = std::bit_cast<std::int32_t>(value);
			param.int_value = static_cast<std::int32_t>(value);
			params.emplace_back(std::move(param));
		}
		void add_string(std::vector<Param>& params)
		{
			Param param{};
			param.kind = Param::Kind::String;
			param.text = next_string();
			params.emplace_back(std::move(param));
		}
		void add_object(std::vector<Param>& params)
		{
			// No object references through the Android extension API; the ACE gets the zeroed block.
			Param param{};
			param.kind = Param::Kind::Object;
			param.object = nullptr;
			params.emplace_back(std::move(param));
		}
		void set_ace(void* const ace_ptr) noexcept { ace = ace_ptr; }

		// The runtime evaluates Fusion expressions and hands the value over as a float; the
		// integer view is derived from it, exactly like the Windows runtime's
		// RFUNCTION_GET_PARAM for a float parameter.
		float next_float()
		{
			switch(kind)
			{
				case AceKind::action_id:     return rt.act_getParamExpFloat ? rt.act_getParamExpFloat(ext, ace) : 0.0f;
				case AceKind::condition_id:  return rt.cnd_getParamExpFloat ? rt.cnd_getParamExpFloat(ext, ace) : 0.0f;
				case AceKind::expression_id: return rt.exp_getParamFloat    ? rt.exp_getParamFloat(ext, ace)    : 0.0f;
			}
			return 0.0f;
		}

		std::string next_string()
		{
			RuntimeFunctions::string value{};
			switch(kind)
			{
				case AceKind::action_id:     value = rt.act_getParamExpString ? rt.act_getParamExpString(ext, ace) : RuntimeFunctions::string{}; break;
				case AceKind::condition_id:  value = rt.cnd_getParamExpString ? rt.cnd_getParamExpString(ext, ace) : RuntimeFunctions::string{}; break;
				case AceKind::expression_id: value = rt.exp_getParamString    ? rt.exp_getParamString(ext, ace)    : RuntimeFunctions::string{}; break;
			}
			if(!value.ptr)
			{
				return {};
			}
			std::string text{value.ptr};
			if(rt.freeString)
			{
				rt.freeString(ext, value);
			}
			return text;
		}

		// Expression parameters: the ACE asks for a type (TYPE_INT/TYPE_STRING/TYPE_FLOAT) and the
		// frame converts, like the Windows runtime's RFUNCTION_GETPARAM.
		std::intptr_t next_expression(std::int32_t const type)
		{
			switch(type)
			{
				case 0: // TYPE_LONG / TYPE_INT
					return static_cast<std::intptr_t>(static_cast<std::int32_t>(next_float()));
				case 1: // TYPE_STRING
					return reinterpret_cast<std::intptr_t>(strings.emplace_back(next_string()).c_str());
				case 2: // TYPE_FLOAT: the runtime's float's bit pattern, as on Windows
					return static_cast<std::intptr_t>(std::bit_cast<std::int32_t>(next_float()));
				default:
					return 0;
			}
		}

	private:
		RuntimeFunctions& rt;
		void* ext;
		AceKind kind;
		void* ace{};
		ParamSource source{};
		std::deque<std::string> strings; // keeps TYPE_STRING expression parameters alive
	};

	// ---------------------------------------------------------------------------------------------
	// Edit data: the runtime passes the serialized edit data the editor wrote.  On Windows the
	// runtime passes the blob starting at its extHeader, which is what SerializedEditData models;
	// the Android loader's exact pointer convention is not documented, so accept both the
	// "blob with header" and "payload only" forms by sanity checking the header.
	// (See docs/COMPATIBILITY.md.)
	// ---------------------------------------------------------------------------------------------
	[[nodiscard]] bool edit_header_is_plausible(extHeader const& header) noexcept
	{
		constexpr std::uint32_t max_size{64u * 1024u * 1024u};
		return (header.extVersion == SerializedEditData::INIPP_V1_ANSI || header.extVersion == SerializedEditData::INIPP_V2_UNICODE)
		    && header.extSize >= sizeof(extHeader)
		    && header.extSize <= max_size;
	}

	[[nodiscard]] SerializedEditData const* blob_from(void* const ed_ptr) noexcept
	{
		if(!ed_ptr)
		{
			return nullptr;
		}
		if(edit_header_is_plausible(*static_cast<extHeader const*>(ed_ptr)))
		{
			return static_cast<SerializedEditData const*>(ed_ptr);
		}
		// Payload-only form: the header sits immediately before the pointer.
		auto const* const before{reinterpret_cast<extHeader const*>(static_cast<std::byte const*>(ed_ptr) - sizeof(extHeader))};
		if(edit_header_is_plausible(*before))
		{
			return reinterpret_cast<SerializedEditData const*>(static_cast<std::byte const*>(ed_ptr) - offsetof(SerializedEditData, raw));
		}
		::ipp_android::log("Ini++: the edit data does not start with an Ini++ object header; using default properties");
		return nullptr;
	}
}

// -------------------------------------------------------------------------------------------------
// The extension object.  Everything shared with the Windows build lives in RunData; this class
// only owns the storage the Windows runtime would allocate through GetRunObjectDataSize().
// -------------------------------------------------------------------------------------------------
struct Extension final
{
	alignas(::RunData) unsigned char storage[sizeof(::RunData)]{};
	::RunData* run_data{};
	// The run header the shared CreateRunObject()/DestroyRunObject() read; Android has no code
	// pages, so it reports 0 (see AndroidRuntime.cpp).
	::CRunApp app{};
	::RunHeader run_header{};

	bool construct(void* const ed_ptr) noexcept
	{
		std::memset(storage, 0, sizeof(storage));
		run_data = reinterpret_cast<RunData*>(storage);
		run_header.rhApp = &app;
		run_data->rHo.hoAdRunHeader = &run_header;
		if(CreateRunObject(run_data, ipp_android::blob_from(ed_ptr), nullptr) != FUSION_FINISH_RUNTIME_STRUCTURE_CONSTRUCTION_SUCCESS)
		{
			::ipp_android::log("Ini++: CreateRunObject() failed");
			run_data = nullptr;
			return false;
		}
		return true;
	}

	~Extension()
	{
		if(run_data)
		{
			// fast = false: give the object the chance to autosave, as the Windows runtime does
			DestroyRunObject(run_data, 0);
			run_data = nullptr;
		}
	}

	static int numberOfConditions() noexcept { return ipp_android::number_of_conditions; }

	int tick() noexcept
	{
		if(!run_data)
		{
			return REFLAG_ONESHOT;
		}
		return HandleRunObject(run_data);
	}

	void run_action(int const num, void* const act)
	{
		if(!run_data)
		{
			return;
		}
		ipp_android::AceParamReader reader{*runtime, ext, ipp_android::AceKind::action_id};
		reader.set_ace(act);
		auto& frame{ipp_android::current_frame()};
		frame.set_source(reader.params());
		(void)ipp_android::dispatch_action(num, run_data, frame, reader);
		frame.set_source(nullptr);
	}

	bool run_condition(int const num, void* const cnd)
	{
		if(!run_data)
		{
			return false;
		}
		ipp_android::AceParamReader reader{*runtime, ext, ipp_android::AceKind::condition_id};
		reader.set_ace(cnd);
		auto& frame{ipp_android::current_frame()};
		frame.set_source(reader.params());
		bool const result{ipp_android::dispatch_condition(num, run_data, frame, reader) != 0};
		frame.set_source(nullptr);
		return result;
	}

	void run_expression(int const num, void* const exp)
	{
		if(!run_data)
		{
			return;
		}
		ipp_android::AceParamReader reader{*runtime, ext, ipp_android::AceKind::expression_id};
		reader.set_ace(exp);
		run_data->rHo.hoFlags = 0;
		auto& frame{ipp_android::current_frame()};
		frame.set_source(reader.params());
		auto const result{ipp_android::dispatch_expression(num, run_data, frame, reader)};
		frame.set_source(nullptr);
		// Same convention as the Windows runtime (ACEs.cpp): HOF_STRING means the returned value
		// is a NUL terminated string, HOF_FLOAT means it is a float's bit pattern in an int32,
		// otherwise it is a plain integer.
		if((run_data->rHo.hoFlags & HOF_STRING) != 0)
		{
			if(runtime->exp_setReturnString)
			{
				auto const* const text{reinterpret_cast<char const*>(static_cast<std::intptr_t>(result))};
				runtime->exp_setReturnString(ext, exp, text ? text : "");
			}
		}
		else if((run_data->rHo.hoFlags & HOF_FLOAT) != 0)
		{
			if(runtime->exp_setReturnFloat)
			{
				runtime->exp_setReturnFloat(ext, exp, std::bit_cast<float>(static_cast<std::int32_t>(result)));
			}
		}
		else if(runtime->exp_setReturnInt)
		{
			runtime->exp_setReturnInt(ext, exp, static_cast<int>(result));
		}
	}

	// Set by the exported createRunObject() before construct()
	RuntimeFunctions* runtime{};
	void* ext{};
};

namespace
{
	// ---------------------------------------------------------------------------------------------
	// The real bodies, shared by both sets of exported names.
	// ---------------------------------------------------------------------------------------------
	void ext_init()
	{
		// Called once when the runtime loads the extension; the shared implementation keeps all
		// state per object, so there is nothing global to set up.
	}

	void* create_run_object(void* const ed_ptr, void* const ext, RuntimeFunctions* const runtime)
	{
		auto* const object{new (std::nothrow) Extension{}};
		if(!object)
		{
			ipp_android::log("Ini++: out of memory while creating the object");
			return nullptr;
		}
		object->runtime = runtime;
		object->ext = ext;
		if(!object->construct(ed_ptr))
		{
			delete object;
			return nullptr;
		}
		return object;
	}

	int get_number_of_conditions()
	{
		return Extension::numberOfConditions();
	}

	void destroy_run_object(Extension* const object)
	{
		delete object;
	}

	int handle_run_object(Extension* const object)
	{
		return object ? object->tick() : REFLAG_ONESHOT;
	}

	void action(Extension* const object, int const num, void* const act)
	{
		if(object)
		{
			object->run_action(num, act);
		}
	}

	bool condition(Extension* const object, int const num, void* const cnd)
	{
		return object != nullptr && object->run_condition(num, cnd);
	}

	void expression(Extension* const object, int const num, void* const exp)
	{
		if(object)
		{
			object->run_expression(num, exp);
		}
	}
}

// -------------------------------------------------------------------------------------------------
// Exports.  The extension name "INI++" is not a valid C identifier, so the entry points the runtime
// derives from INI++.mfx are exported through assembler aliases of the ordinary functions; the
// same functions are exported again under the plain identifier the runtime derives from an MFX
// whose name is a valid identifier (the default: CRunIniPlusPlus, from IniPlusPlus.mfx).  Renaming
// the MFX to another valid identifier needs nothing more than
// -DIPPP_ANDROID_EXT_NAME=CRunThatName for this file (tools/build_android.sh passes it through).
// -------------------------------------------------------------------------------------------------
#define IPPP_ANDROID_EXPORT __attribute__((visibility("default")))

#ifndef IPPP_ANDROID_EXT_NAME
#define IPPP_ANDROID_EXT_NAME CRunIniPlusPlus
#endif

#define IPPP_STR_(x) #x
#define IPPP_STR(x) IPPP_STR_(x)
#define IPPP_CAT_(a, b) a##b
#define IPPP_CAT(a, b) IPPP_CAT_(a, b)
// The plain identifier of an entry point, e.g. CRunIniPlusPlus_action
#define IPPP_ENTRY(name) IPPP_CAT(IPPP_ANDROID_EXT_NAME, IPPP_CAT(_, name))
// Its spelling with the extension's printable name, e.g. "CRunINI++_action" is passed in as text.
#define IPPP_DEFINE_ENTRY_POINTS_()                                                                                  \
	extern "C" IPPP_ANDROID_EXPORT void IPPP_ENTRY(extInit)()                              { ::ext_init(); }         \
	extern "C" IPPP_ANDROID_EXPORT void* IPPP_ENTRY(createRunObject)(void* edPtr, void* ext, RuntimeFunctions* fn)    \
	                                                                                       { return ::create_run_object(edPtr, ext, fn); } \
	extern "C" IPPP_ANDROID_EXPORT int IPPP_ENTRY(getNumberOfConditions)()                 { return ::get_number_of_conditions(); } \
	extern "C" IPPP_ANDROID_EXPORT void IPPP_ENTRY(destroyRunObject)(Extension* e)         { ::destroy_run_object(e); } \
	extern "C" IPPP_ANDROID_EXPORT int IPPP_ENTRY(handleRunObject)(Extension* e)           { return ::handle_run_object(e); } \
	extern "C" IPPP_ANDROID_EXPORT void IPPP_ENTRY(action)(Extension* e, int n, void* p)   { ::action(e, n, p); }    \
	extern "C" IPPP_ANDROID_EXPORT bool IPPP_ENTRY(condition)(Extension* e, int n, void* p) { return ::condition(e, n, p); } \
	extern "C" IPPP_ANDROID_EXPORT void IPPP_ENTRY(expression)(Extension* e, int n, void* p) { ::expression(e, n, p); }

IPPP_DEFINE_ENTRY_POINTS_()

// Assembler aliases for the runtime's spelling of the extension name.  Quoted symbol names are
// accepted by gas and by the LLVM integrated assembler, and the aliases end up in .dynsym like any
// other exported function (tools/smoke_build_so.sh and tools/build_android.sh both verify this:
// nm -D and dlsym must find all 16 names, and the Android build fails if they are missing).
#ifndef IPPP_ANDROID_NO_MANGLED_SYMBOLS
#define IPPP_ASM_ALIAS(mangled, real) ".globl \"" mangled "\"\n.set \"" mangled "\", " real "\n"
#define IPPP_MANGLED_ENTRY(mangled, name) IPPP_ASM_ALIAS(mangled, IPPP_STR(IPPP_ENTRY(name)))
__asm__(IPPP_MANGLED_ENTRY("CRunINI++_extInit", extInit)
        IPPP_MANGLED_ENTRY("CRunINI++_createRunObject", createRunObject)
        IPPP_MANGLED_ENTRY("CRunINI++_getNumberOfConditions", getNumberOfConditions)
        IPPP_MANGLED_ENTRY("CRunINI++_destroyRunObject", destroyRunObject)
        IPPP_MANGLED_ENTRY("CRunINI++_handleRunObject", handleRunObject)
        IPPP_MANGLED_ENTRY("CRunINI++_action", action)
        IPPP_MANGLED_ENTRY("CRunINI++_condition", condition)
        IPPP_MANGLED_ENTRY("CRunINI++_expression", expression));
#endif // IPPP_ANDROID_NO_MANGLED_SYMBOLS

// -------------------------------------------------------------------------------------------------
// ACE dispatch: one case per ACE, reading the parameters Menus.cpp declares for it and calling the
// shared body from ACEs.cpp.  Generated by tools/gen_android_aces.py.
// -------------------------------------------------------------------------------------------------
namespace ipp_android
{
#include "AceDispatch.inc"
}
