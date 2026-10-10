# Build the project-owned JNI bridge loaded by the Java CRunExtension adapter.
#
# This is NOT a Clickteam RuntimeNative extension: it does not include or imitate RuntimeNative.h.
# The Fusion 2.5 Java runtime registers CRunIniPlusPlus through CExtLoad and loads this library as
# an ordinary mmf/<abi> native object before the Java extension is instantiated.

LOCAL_PATH := $(call my-dir)
IPPP_ROOT  := $(LOCAL_PATH)/../..

include $(CLEAR_VARS)

LOCAL_MODULE        := IniPlusPlusBridge
LOCAL_CPP_EXTENSION := .cpp
LOCAL_SRC_FILES := \
	../runtime/Bridge.cpp \
	JniBridge.cpp \
	../../Runtime.cpp \
	../../ACEs.cpp \
	../runtime/AndroidRuntime.cpp \
	../fusion/lSDK/UnicodeUtilities.cpp

LOCAL_CPPFLAGS := \
	-std=c++20 \
	-DFUSION_ANDROID_RUNTIME \
	-DFUSION_ACTIONS -DFUSION_CONDITIONS -DFUSION_EXPRESSIONS \
	-DNDEBUG \
	-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES := \
	$(IPPP_ROOT) \
	$(IPPP_ROOT)/android/fusion \
	$(IPPP_ROOT)/android/runtime

LOCAL_CFLAGS   := -fvisibility=hidden
LOCAL_LDFLAGS  := -Wl,--gc-sections -Wl,--no-undefined -Wl,-z,max-page-size=16384
LOCAL_LDLIBS   := -llog

include $(BUILD_SHARED_LIBRARY)
