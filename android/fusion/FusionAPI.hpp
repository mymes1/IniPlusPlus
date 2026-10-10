#ifndef FusionAPI_HeaderPlusPlus
#define FusionAPI_HeaderPlusPlus

/*
 * Ini++ Android source-adaptation layer for the shared Windows/MFX C++ sources.
 *
 * Windows builds use Clickteam's Windows extension SDK through lSDK. Android does not compile the
 * MFX ABI. Instead, android/java/Extensions/CRunIniPlusPlus.java reads ACE parameters using the
 * public Java CRunExtension API, android/jni/JniBridge.cpp translates them across JNI, and the
 * Java-independent Bridge.cpp dispatches the existing ACE bodies. This header supplies the small
 * Fusion/lSDK-shaped types and helpers those shared C++ files expect; it is not RuntimeNative.h,
 * does not reproduce Clickteam's private native ABI, and is not sufficient by itself to load an
 * extension in Fusion.
 *
 * The executable Android integration is Java CRunExtension + this project's JNI library. This
 * compatibility layer owns the platform-side parameter frame, sandbox file access and edit-data
 * decoding. Anything not represented by the current Java adapter (notably object/custom parameter
 * payloads) must remain documented as a limitation; a host compile is not an exporter build.
 */

#include "lSDK/UnicodeUtilities.hpp"
#include "ParamFrame.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

// -------------------------------------------------------------------------------------------------
// Win32 scalar types and macros used by the shared sources.  These are declaration-level
// conveniences only; no Win32 function is ever called.
// -------------------------------------------------------------------------------------------------
using TCHAR = lSDK::char_t;
using LPTSTR = TCHAR*;
using LPCTSTR = TCHAR const*;
using LPSTR = char*;
using LPCSTR = char const*;
// On Windows these are wchar_t pointers; in the Android build the extension's own characters are
// the native ones, so they alias the platform character type (the only use is the alterable string
// reflection code, which never runs on Android).
using LPWSTR = TCHAR*;
using LPCWSTR = TCHAR const*;
#define CALLBACK
using BOOL = int;
using DWORD = std::uint32_t;
using UINT = std::uint32_t;
using WPARAM = std::uintptr_t;
using LPARAM = std::intptr_t;
using HANDLE = void*;
using HGLOBAL = void*;
using HMENU = void*;
using HMODULE = void*;
using HINSTANCE = void*;
using WNDPROC = std::intptr_t (*)(void*, UINT, WPARAM, LPARAM);
struct RECT { std::int32_t left{}, top{}, right{}, bottom{}; };
struct HWND__;
using HWND = HWND__*;

#define WINAPI
#define FUSION_API

#ifndef _T
#define _T(x) x
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0
#endif

#define LOWORD(v) static_cast<std::uint16_t>(static_cast<std::uint32_t>(v) & 0xFFFFu)
#define HIWORD(v) static_cast<std::uint16_t>((static_cast<std::uint32_t>(v) >> 16) & 0xFFFFu)
#define MAKELONG(a, b) static_cast<std::int32_t>(static_cast<std::uint32_t>(LOWORD(a)) | (static_cast<std::uint32_t>(LOWORD(b)) << 16))

// The Windows SDK declares these types before the extension headers use them.
struct RunData;
struct RunObject;
struct RunHeader;
struct CRunApp;
struct SerializedEditData;
struct EDITDATA;
struct EditData;
// The extension edit header at the start of the serialized edit data.  The layout is the one the
// Windows SDK documents (Cncf.h: five 32-bit fields) because the blob in the MFA is written by the
// Windows editor; sizeof(extHeader) is therefore 20 on every platform.
struct extHeader final
{
	std::uint32_t extSize{}, extMaxSize{}, extVersion{};
	std::uint32_t extID{};          // LPVOID in the 32-bit Windows SDK; unused by Ini++
	std::uint32_t extPrivateData{}; // ditto
};
static_assert(sizeof(extHeader) == 20);

// -------------------------------------------------------------------------------------------------
// Fusion constants (values match the Windows SDK; see lSDK/include/FusionAPI/Cncf.h)
// -------------------------------------------------------------------------------------------------
#define TYPE_LONG   0x0000
#define TYPE_INT    TYPE_LONG
#define TYPE_STRING 0x0001
#define TYPE_FLOAT  0x0002
#define TYPE_DOUBLE TYPE_FLOAT

#define HOF_TRUEEVENT 0x0002
#define HOF_FLOAT     0x4000
#define HOF_STRING    0x8000

#define RFUNCTION_GET_PARAM          0
#define RFUNCTION_GET_PARAM_FLOAT    2
#define RFUNCTION_GETSTRINGSPACE_EX  3
#define RFUNCTION_GENERATEEVENT      4
#define RFUNCTION_SETPOSITION        17
#define RFUNCTION_SET_STRING         19

#define CHARENC_DEFAULT 0

#define VALUE_NUMBEROF_ALTERABLE   10
#define VALUES_NUMBEROF_ALTERABLE  10
#define STRINGS_NUMBEROF_ALTERABLE 10

#define OEFLAG_VALUES            0x0001
#define OEFLAG_SPRITES           0x0002
#define OEFLAG_MOVEMENTS         0x0004
#define OEFLAG_ANIMATIONS        0x0008
#define OEFLAG_NEVERKILL         0x0100
#define OEFLAG_RUNBEFOREFADEIN   0x4000

#define FUSION_OEFLAGS (OEFLAG_VALUES|OEFLAG_RUNBEFOREFADEIN|OEFLAG_NEVERKILL|OEFLAG_MOVEMENTS)

// Fusion application flags (used by Runtime.cpp's default folder lookup)
#define GA_ONEFILE 0x0001

// -------------------------------------------------------------------------------------------------
// The Android runtime's own entry points; implemented in android/runtime/AndroidRuntime.cpp
// -------------------------------------------------------------------------------------------------
namespace ipp_android
{
	[[nodiscard]] ParamFrame& current_frame() noexcept; // valid while an ACE body runs
	// A zeroed object/custom-parameter block for values the current Java/JNI adapter does not yet
	// marshal. The public Java API exposes these values, but the C++ bridge currently uses safe
	// defaults instead of pretending to have a native Fusion C++ object.
	[[nodiscard]] void* empty_object() noexcept;
	void log(std::string_view text) noexcept;           // goes to the Android log/stderr
	[[nodiscard]] std::filesystem::path data_dir();      // the app's writable sandbox directory
}

// Parameter accessors.  On Windows these are the SDK's macros reading from the runtime's parameter
// cursor; here they read from the current ACE's parameter frame (same order, same semantics).
#define CNC_GetParameter(rd)                                   (::ipp_android::current_frame().cnc_get_object())
#define CNC_GetIntParameter(rd)                                (::ipp_android::current_frame().cnc_get_int())
#define CNC_GetStringParameter(rd)                             (::ipp_android::current_frame().cnc_get_string())
#define CNC_GetFloatParameter(rd)                              (::ipp_android::current_frame().cnc_get_float())
#define CNC_GetFirstExpressionParameter(rd, params_handle, ty) ((void)(params_handle), ::ipp_android::current_frame().cnc_get_expression(ty))
#define CNC_GetNextExpressionParameter(rd, params_handle, ty)  ((void)(params_handle), ::ipp_android::current_frame().cnc_get_expression(ty))

// Custom parameter block, as declared by Cncf.h on Windows. The Windows definition has
// `char pextData[2]` with the extension's payload laid out after it; Android has no custom
// parameter payload, so Ini++ reads the documented defaults (zeroes) out of this zeroed block.
struct paramExt final
{
	std::int16_t pextSize{};
	std::int16_t pextType{};
	std::int16_t pextCode{};
	char pextData[64]{};
};

#define NOT_YET_IMPLEMENTED_ANDROID(func) ::ipp_android::log(std::string{"Ini++: ACE not implemented on Android: "} + (func))

namespace fusion
{
	using boolean = std::int32_t;
	using string_buffer = TCHAR*;
	using string_view = TCHAR const*;
	using string_array_view = string_view const*;
	using ace_info_array = std::int16_t const*;

	using action_return_t = std::int16_t;
	using condition_return_t = std::int32_t;
	using expression_return_t = std::intptr_t;
	using ac_param_t = std::intptr_t;
	using e_params_t = std::intptr_t;

	using action_func = action_return_t FUSION_API (*)(RunData* const, ac_param_t const, ac_param_t const) noexcept;
	using condition_func = condition_return_t FUSION_API (*)(RunData* const, ac_param_t const, ac_param_t const) noexcept;
	using expression_func = expression_return_t FUSION_API (*)(RunData* const, e_params_t const) noexcept;
	using action_func_pointer = action_func;
	using condition_func_pointer = condition_func;
	using expression_func_pointer = expression_func;

	// lSDK's structure checks validate Microsoft's ABI layouts, which do not exist on Android.
	constexpr bool is_valid_editdata_structure() noexcept { return true; }
	template<std::int32_t Flags, typename RunDataT = RunData>
	constexpr bool is_valid_rundata_structure() noexcept { return true; }
}

// -------------------------------------------------------------------------------------------------
// Fusion runtime structures: only the members the shared Ini++ code actually reads are present.
// The current bridge only constructs the shared Ini++ run object; it does not yet mirror other
// Java Fusion objects' alterable-value blocks (see docs/COMPATIBILITY.md).
// -------------------------------------------------------------------------------------------------
struct CValue final
{
	std::int32_t m_type{};
	std::int32_t m_long{};
	double m_double{};
	union { void* m_pObject{}; };
};

// Version/build masks from the Windows SDK (Cncy.h); Ini++ uses them to pick the alterable-value
// layout when it receives a RunObject. The current Java/JNI bridge does not populate proxies for
// other Java objects, so object-property ACEs are guarded as documented in docs/COMPATIBILITY.md.
#define MMFVERSION_MASK 0xFFFF0000
#define MMFBUILD_MASK   0x00000FFF
#define MMFVERSION_20   0x02000000
#define MMFVERSION_25   0x02050000

struct mv final
{
	std::uint32_t version{};
	bool unicode{true};
	std::uint32_t codepage{};

	// Windows exposes these as function pointers in the mv struct; the Android runtime has no
	// equivalent, so they report a modern Fusion 2.5 build and do nothing.
	std::int32_t mvGetVersion() const noexcept { return MMFVERSION_25 | 292; }
};

struct rCom final
{
	std::int32_t rcChanged{}, rcDir{}, rcSpeed{}, rcAnim{}, rcImage{};
};

struct rMvt final {};

struct rVal final
{
	std::int32_t rvNumberOfValues{};
	std::int32_t rvNumberOfStrings{};
	std::int32_t rvValueFlags{};
	std::int32_t* rvpValues{};
	TCHAR** rvpStrings{};
	TCHAR* rvStrings{};
};

struct headerObject;
struct CRunApp;

struct kpxFunction final
{
	std::intptr_t (*routine)(void*, std::int32_t, std::int32_t){};
};

struct rh4_ final
{
	mv* rh4Mv{};
	kpxFunction* rh4KpxFunctions{};
	CValue rh4ExpValue1{};
	CValue rh4ExpValue2{};
	std::uint32_t rh4EventCountOR{};
};

struct RunHeader final
{
	CRunApp* rhApp{};
	std::int32_t rh4FrameCount{};
	rh4_ rh4{};
};

struct RunObject;

// Alterable value layouts of MMF 2.0, 2.5 and 2.5+ (rVal20a/rVal20b/rVal25/rVal25P in the
// Windows SDK). Only the members Ini++ reflects over are present.
struct rVal20a final
{
	TCHAR* rvStrings{};
	std::int32_t rvValueFlags{};
};
struct rVal20b final
{
	CValue* rvpValues{};
	TCHAR* rvStrings{};
	std::int32_t rvValueFlags{};
};
struct rVal25 final
{
	std::int32_t rvNumberOfValues{};
	CValue* rvpValues{};
	TCHAR* rvStrings{};
	std::int32_t rvValueFlags{};
};
struct rVal25P final
{
	std::int32_t rvNumberOfValues{};
	std::int32_t rvNumberOfStrings{};
	CValue* rvpValues{};
	TCHAR* rvStrings{};
	TCHAR** rvpStrings{};
	std::int32_t rvValueFlags{};
};

struct headerObject final
{
	RunHeader* hoAdRunHeader{};
	std::int32_t hoX{}, hoY{};
	std::int32_t hoEventNumber{};
	std::int32_t hoFlags{};
	std::int32_t hoOEFlags{};
	std::int32_t hoOffsetValue{};
	RunObject* hoNext{};
};

struct RunObject final
{
	headerObject roHo{};
	rCom roc{};
	rMvt rom{};
	rVal rov{};
	RunObject* rop{};
};

struct createObjectInfo final
{
	std::int32_t cobX{}, cobY{}, cobDir{}, cobLayer{};
	std::int32_t cobFlags{};
};

struct CRunApp final
{
	CRunApp* m_pParentApp{};
	char const* m_appFileName{};
	char const* m_editorFileName{};
	std::uint32_t m_dwCodePage{};
	struct { std::uint32_t gaFlags{}; } m_hdr;
};

// The `mv` accessors are no-ops: on Android the runtime does not expose the Windows memory/version
// manager, and Ini++ only uses it for object reflection, which is unsupported on this platform.
inline std::int32_t mvGetVersion(mv* const) noexcept { return MMFVERSION_25 | 292; }
inline bool mvIsUnicodeVersion(mv* const) noexcept { return true; }
inline std::int32_t mvGetAppCodePage(mv* const, ::CRunApp const*) noexcept { return 0; }
inline void* mvMalloc(mv* const, std::size_t const size) noexcept { return std::calloc(1, size); }
inline void mvFree(mv* const, void* const p) noexcept { std::free(p); }
inline void* mvReAlloc(mv* const, void* const p, std::size_t const size) noexcept { return std::realloc(p, size); }
inline void mvInsertProps(mv* const, void* const) noexcept {}
inline void mvRefreshProp(mv* const, void* const, std::int32_t) noexcept {}
inline void* mvReAllocEditData(mv* const, void* const p, std::size_t const size) noexcept { return std::realloc(p, size); }

// Files are accessed through the app sandbox on Android (implemented in
// android/runtime/AndroidRuntime.cpp).  They return malloc()ed memory (free via mvFree/CoTaskMemFree
// as on Windows) and behave like the Windows runtime's file helpers: nullptr on failure, and the
// text file helpers transparently handle UTF-8, UTF-16 BOMs and the system code page.
TCHAR* mvLoadTextFile(mv* const mV, TCHAR const* const filename, std::int32_t const encoding, std::int32_t const reserved) noexcept;
std::int32_t mvSaveTextFile(mv* const mV, TCHAR const* const filename, TCHAR const* const text, std::int32_t const encoding) noexcept;

// PWSTR: on Windows this is wchar_t*; here it is char* because Android paths are UTF-8 byte
// strings, which is also what std::filesystem::path consumes on this platform.
using PWSTR = char*;

// The "known folder" API that Runtime.cpp uses lives in android/fusion/Shlobj.h.
#include "Shlobj.h"

// Strings returned by expressions are allocated from the frame's pool (Windows: the runtime's
// string space via RFUNCTION_GETSTRINGSPACE_EX).
inline void* callRunTimeFunction(RunData* const /*run_data*/, std::int32_t const function, std::intptr_t const /*param0*/, std::intptr_t const buffer_size) noexcept
{
	if(function == RFUNCTION_GETSTRINGSPACE_EX)
	{
		return ::ipp_android::current_frame().allocate_return_string(static_cast<std::size_t>(buffer_size));
	}
	return nullptr;
}

// -------------------------------------------------------------------------------------------------
// Fusion entry-point feature macros. Runtime.cpp implements the shared lifecycle code; the JNI
// library calls the same C++ lifecycle from Bridge.cpp, behind the Java CRunExtension adapter.
// -------------------------------------------------------------------------------------------------
#define FUSION_GET_RUNTIME_STRUCTURE_SIZE

#define FUSION_FINISH_RUNTIME_STRUCTURE_CONSTRUCTION
#define FUSION_FINISH_RUNTIME_STRUCTURE_CONSTRUCTION_FAILURE 1
#define FUSION_FINISH_RUNTIME_STRUCTURE_CONSTRUCTION_SUCCESS 0

#define FUSION_BEGIN_RUNTIME_STRUCTURE_DESTRUCTION
#define FUSION_BEGIN_RUNTIME_STRUCTURE_DESTRUCTION_SUCCESS 0

#define FUSION_ON_TICK
#define FUSION_ON_TICK_STOP_CALLING 2 // REFLAG_ONESHOT
#define FUSION_ON_TICK_DONT_DRAW    0
#define FUSION_ON_TICK_CALL_DRAW    1 // REFLAG_DISPLAY

#define FUSION_ON_DRAW
#define FUSION_ON_DRAW_SUCCESS 0

#define FUSION_SERIALIZE_RUNTIME
#define FUSION_SERIALIZE_RUNTIME_FAILURE FALSE
#define FUSION_SERIALIZE_RUNTIME_SUCCESS TRUE

#define FUSION_DESERIALIZE_RUNTIME
#define FUSION_DESERIALIZE_RUNTIME_FAILURE FALSE
#define FUSION_DESERIALIZE_RUNTIME_SUCCESS TRUE

#define FUSION_PAUSE_RUNTIME
#define FUSION_PAUSE_RUNTIME_SUCCESS 0
#define FUSION_UNPAUSE_RUNTIME
#define FUSION_UNPAUSE_RUNTIME_SUCCESS 0

#define FUSION_START_APPLICATION
#define FUSION_END_APPLICATION

#define FUSION_CONDITIONS_FALSE 0
#define FUSION_CONDITIONS_TRUE  1

#endif
