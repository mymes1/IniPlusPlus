#ifndef RuntimeNative_Shim_HeaderPlusPlus
#define RuntimeNative_Shim_HeaderPlusPlus

/*
 * Ini++ Android runtime: stand-in for Clickteam's <RuntimeNative.h>.
 *
 * ============================  READ ME FIRST  ============================
 *
 * The real `RuntimeNative.h` defines the `RuntimeFunctions` table that the Fusion 2.5 Android
 * runtime passes to every native extension.  It is NOT part of the public GitHub SDK
 * (github.com/ClickteamLLC/android); it ships inside
 * `<Fusion 2.5 install>/Data/Runtime/Android/RuntimeAndroid.zip`, which is proprietary and
 * cannot be redistributed here.
 *
 * This file is a *documented reconstruction* of the parts of that interface Ini++ needs, built
 * from:
 *   * the official native extension template (`NativeExtension.h`/`NativeExtension.cc`, which
 *     calls every member below) - names and signatures,
 *   * the Java-side bridge of the runtime (`Extensions.CRunNativeExtension`,
 *     `Runtime.CRunExtension`, `Runtime.Native` in the open-source CTFAK reimplementation of the
 *     runtime) - the calling sequence and the ACE/expression parameter model, and
 *   * the exported symbol table of `libRuntimeNative.so` (the runtime's own implementation of
 *     these functions, e.g. `act_getParamExpString(Instance*, _jobject*)`), which confirms the
 *     member signatures and their C++ types.
 *
 * It exists so that the Ini++ Android sources can be compiled, link-checked and unit-tested on any
 * machine, and so that the build fails with a clear message if the real header is missing.
 *
 * *** The member ORDER of RuntimeFunctions is only known to the proprietary header.** ***
 * A shared object built against this shim is a smoke/CI artifact only: it must NOT be shipped to
 * users.  Release builds must pass the extracted official SDK directory
 * (`-DIPPP_ANDROID_SDK_DIR=...`, see docs/BUILD.md); the packaging tool refuses to build a release
 * package without it.
 */

#include <cstdint>

struct RuntimeFunctions final
{
	// A string owned by the runtime. `ptr` is a NUL terminated UTF-8 C string; release it with
	// freeString() when the ACE/expression call has returned (the official template's ACE wrapper
	// does exactly that in its destructor).
	struct string final
	{
		char* ptr;
		std::int32_t length;
		std::int32_t type;
	};

	// --- string lifetime ---
	void (*freeString)(void* ext, string s);

	// --- action parameters (void* act = the action object) ---
	int (*act_getParamExpression)(void* ext, void* act);
	string (*act_getParamExpString)(void* ext, void* act);
	float (*act_getParamExpFloat)(void* ext, void* act);

	// --- condition parameters (void* cnd = the condition object) ---
	int (*cnd_getParamExpression)(void* ext, void* cnd);
	string (*cnd_getParamExpString)(void* ext, void* cnd);
	float (*cnd_getParamExpFloat)(void* ext, void* cnd);

	// --- expression parameters and return values (void* exp = the expression object) ---
	int (*exp_getParamInt)(void* ext, void* exp);
	string (*exp_getParamString)(void* ext, void* exp);
	float (*exp_getParamFloat)(void* ext, void* exp);
	void (*exp_setReturnInt)(void* ext, void* exp, int value);
	void (*exp_setReturnString)(void* ext, void* exp, const char* value);
	void (*exp_setReturnFloat)(void* ext, void* exp, float value);

	// --- events ---
	void (*generateEvent)(void* ext, int code, int param);
};

// Object update flags returned by <ext>_handleRunObject()
constexpr int REFLAG_DISPLAY{1};
constexpr int REFLAG_ONESHOT{2};

#endif
