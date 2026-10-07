// Host-test stand-in for the Clickteam Fusion 2.5 Android SDK's <RuntimeNative.h>.
//
// *** THIS IS NOT THE OFFICIAL SDK HEADER. ***  It exists so that the extension sources can be
// compiled and exercised on a development machine or in CI without the proprietary SDK download,
// and it deliberately declares only the part of the interface the extension uses, exactly as the
// official NativeExtension.h uses it (by member name):
//
//     freeString, act_getParam{Expression,ExpString,ExpFloat}, cnd_getParam*, exp_getParam*,
//     exp_setReturn{Int,String,Float}, generateEvent        (see the Android SDK's
//     native/base/NativeExtension.h)
//
// The real RuntimeFunctions struct has many more members; the extension never indexes it, it only
// calls these members by name, so a build against the real <RuntimeNative.h> from
// https://www.clickteam.com/extensions-sdks behaves identically.
//
// Real Android builds must use the SDK header: pass -DIPPP_ANDROID_SDK_DIR=<extracted SDK> to
// tools/build_android.sh, or fetch it in CI (see docs/BUILD.md).  Nothing built with this file may
// be distributed as an installable extension.

#ifndef IPPP_MOCK_RUNTIME_NATIVE_H
#define IPPP_MOCK_RUNTIME_NATIVE_H

#if !defined(IPPP_HOST_TESTS) && !defined(IPPP_MOCK_ANDROID_SDK)
#error "android/tests/mock-sdk/RuntimeNative.h is a host-test stand-in, not the Clickteam SDK. \
Define IPPP_HOST_TESTS (host tests) or IPPP_MOCK_ANDROID_SDK (build smoke test), or point the \
build at the real SDK header with -DIPPP_ANDROID_SDK_DIR=<extracted SDK>."
#endif

#include <cstdint>

struct RuntimeFunctions
{
	// The official header's string type has at least a `char* ptr` member; extensions read
	// `.ptr` and hand the value back to freeString() unchanged.
	struct string
	{
		char* ptr{};
		std::int32_t length{};
		std::int32_t type{};
	};

	void (*freeString)(void* ext, string s){};

	std::int32_t (*act_getParamExpression)(void* ext, void* act){};
	string (*act_getParamExpString)(void* ext, void* act){};
	float (*act_getParamExpFloat)(void* ext, void* act){};

	std::int32_t (*cnd_getParamExpression)(void* ext, void* cnd){};
	string (*cnd_getParamExpString)(void* ext, void* cnd){};
	float (*cnd_getParamExpFloat)(void* ext, void* cnd){};

	std::int32_t (*exp_getParamInt)(void* ext, void* exp){};
	string (*exp_getParamString)(void* ext, void* exp){};
	float (*exp_getParamFloat)(void* ext, void* exp){};

	void (*exp_setReturnInt)(void* ext, void* exp, int value){};
	void (*exp_setReturnString)(void* ext, void* exp, char const* value){};
	void (*exp_setReturnFloat)(void* ext, void* exp, float value){};

	void (*generateEvent)(void* ext, int code, int param){};
};

// From the official SDK's NativeExtension.h
constexpr int REFLAG_DISPLAY = 1;
constexpr int REFLAG_ONESHOT = 2;

#endif // IPPP_MOCK_RUNTIME_NATIVE_H
