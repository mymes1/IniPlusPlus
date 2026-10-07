#!/usr/bin/env bash
# Build and run the Ini++ Android runtime host tests (see android/tests/HostTests.cpp).
#
# These compile the Android implementation (Runtime.cpp, ACEs.cpp, the Android platform layer and
# the exported extension entry points) with the host compiler against the interface shim in
# android/sdk-shim, and drive it through a mock of the Fusion Android native extension API.
# No Android SDK, NDK or Fusion installation is required.
set -euo pipefail
cd "$(dirname "$0")/.."

CXX="${CXX:-g++}"
BUILD="${BUILD_DIR:-build/android-host-tests}"
# SANITIZE=1 additionally builds with AddressSanitizer/UndefinedBehaviorSanitizer: the tests then
# also prove that nothing reads out of bounds (the mock runtime's parameters, the edit data blob).
SANITIZE="${SANITIZE:-0}"
mkdir -p "$BUILD"
if [ "$SANITIZE" = 1 ]; then
	BUILD="$BUILD/asan"
	mkdir -p "$BUILD"
fi

FLAGS=(-std=c++20 -O1 -g
       -DFUSION_ANDROID_RUNTIME
       -DFUSION_ACTIONS -DFUSION_CONDITIONS -DFUSION_EXPRESSIONS
       -DNDEBUG
       -DIPPP_HOST_TESTS
       -Iandroid/fusion -Iandroid/sdk-shim -Iandroid/runtime -Iandroid/tests/mock-sdk -I.
       -Wall -Wextra -Wno-unused-parameter)

if [ "$SANITIZE" = 1 ]; then
	FLAGS+=(-fsanitize=address,undefined -fno-omit-frame-pointer -O1)
fi

"$CXX" "${FLAGS[@]}" \
    android/tests/HostTests.cpp Runtime.cpp ACEs.cpp android/runtime/AndroidRuntime.cpp \
    android/jni/IniPlusPlusExtension.cc android/fusion/lSDK/UnicodeUtilities.cpp \
    -o "$BUILD/host-tests" -pthread

"$BUILD/host-tests"
