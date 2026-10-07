# Ini++ (Unicode) - Fusion 2.5 Android runtime extension.
#
# Builds the native extension the Android exporter loads from the APK's assets/mmf/<abi>/ folder.
# Use tools/build_android.sh, which locates the Clickteam Android SDK's <RuntimeNative.h>, runs
# ndk-build with this makefile and verifies the exported entry points; invoking ndk-build by hand
# requires -DIPPP_ANDROID_SDK_INC=<folder containing RuntimeNative.h>.
#
# This is the same source NinjaII's Windows MFX is built from: Runtime.cpp and ACEs.cpp are the
# shared implementation, android/fusion is the Windows API shim, android/runtime holds the Android
# platform layer and the generated ACE dispatch table.

LOCAL_PATH := $(call my-dir)
IPPP_ROOT  := $(LOCAL_PATH)/../..

include $(CLEAR_VARS)

# CRunIniPlusPlus is a valid C identifier, so ndk-build can produce libCRunIniPlusPlus.so; the
# package step renames it to CRunINI++.so, the name the runtime derives from INI++.mfx.  Both
# spellings of every entry point are exported.
LOCAL_MODULE        := CRunIniPlusPlus
LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := \
	IniPlusPlusExtension.cc \
	../../Runtime.cpp \
	../../ACEs.cpp \
	../runtime/AndroidRuntime.cpp \
	../fusion/lSDK/UnicodeUtilities.cpp

LOCAL_CPPFLAGS := \
	-std=c++20 \
	-DFUSION_ANDROID_RUNTIME \
	-DFUSION_ACTIONS -DFUSION_CONDITIONS -DFUSION_EXPRESSIONS \
	-DFUSION_ON_TICK -DNDEBUG \
	-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES := \
	$(IPPP_ROOT) \
	$(IPPP_ROOT)/android/fusion \
	$(IPPP_ROOT)/android/runtime \
	$(LOCAL_PATH)

ifneq ($(IPPP_ANDROID_SDK_INC),)
LOCAL_C_INCLUDES += $(IPPP_ANDROID_SDK_INC)
endif

# Extensions share the process with the runtime, so keep everything but the entry points hidden
# (the exported names are marked visibility("default")); the static STL keeps the extension from
# requiring libc++_shared.so, which the Fusion-generated APK does not ship.
LOCAL_CFLAGS   := -fvisibility=hidden
# --no-undefined catches a missing ACE dispatch table or a stray dependency at build time instead
# of at application start-up ("undefined symbol" from the native loader).
LOCAL_LDFLAGS  := -Wl,--gc-sections -Wl,--no-undefined -Wl,-z,max-page-size=16384
LOCAL_LDLIBS   := -llog

include $(BUILD_SHARED_LIBRARY)
