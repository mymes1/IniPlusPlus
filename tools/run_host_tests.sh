#!/usr/bin/env bash
# Build/run tests for the Java/JNI-independent Android bridge (see android/tests/BridgeTests.cpp).
# These tests exercise the shared C++ ACE bodies through the project-owned Java/JNI-independent
# bridge and Android sandbox layer. They do not prove Java compilation or exporter/APK loading.
set -euo pipefail
cd "$(dirname "$0")/.."

CXX="${CXX:-g++}"
BUILD="${BUILD_DIR:-build/android-bridge-tests}"
SANITIZE="${SANITIZE:-0}"

python3 tools/gen_android_aces.py --check
mkdir -p "$BUILD"
if [ "$SANITIZE" = 1 ]; then
	BUILD="$BUILD/asan"
	mkdir -p "$BUILD"
fi

FLAGS=(-std=c++20 -O1 -g
       -DFUSION_ANDROID_RUNTIME
       -DFUSION_ACTIONS -DFUSION_CONDITIONS -DFUSION_EXPRESSIONS
       -DNDEBUG
       -Iandroid/fusion -Iandroid/runtime -I.)

if [ "$SANITIZE" = 1 ]; then
	FLAGS+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi

"$CXX" "${FLAGS[@]}" \
    android/tests/BridgeTests.cpp android/runtime/Bridge.cpp \
    Runtime.cpp ACEs.cpp android/runtime/AndroidRuntime.cpp \
    android/fusion/lSDK/UnicodeUtilities.cpp \
    -o "$BUILD/bridge-tests" -pthread

"$BUILD/bridge-tests"

bash tools/test_android_package.sh
