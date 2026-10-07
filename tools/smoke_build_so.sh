#!/usr/bin/env bash
# Host smoke build of the Android extension as a shared object.
#
# The real Android build needs the NDK *and* the Clickteam Android SDK's <RuntimeNative.h>
# (proprietary, see docs/BUILD.md).  This script builds the exact same sources into a .so with the
# host compiler and the stand-in header in android/tests/mock-sdk, then checks the exported
# symbols.  It cannot validate the Clickteam ABI, but it does prove that
#   * the Android sources compile with the flags used by tools/build_android.sh
#     (C++20, FUSION_ANDROID_RUNTIME, -fvisibility=hidden, exceptions),
#   * the extension links as a shared object, and
#   * the symbol names the runtime looks up (CRunINI++_*) are really exported, including the
#     assembler aliases for the extension name that is not a valid C identifier.
set -euo pipefail
cd "$(dirname "$0")/.."

CXX="${CXX:-g++}"
BUILD="${BUILD_DIR:-build/android-smoke}"
mkdir -p "$BUILD"

FLAGS=(-std=c++20 -O2 -fPIC -shared -fvisibility=hidden
       -Wl,--no-undefined
       -DFUSION_ANDROID_RUNTIME
       -DFUSION_ACTIONS -DFUSION_CONDITIONS -DFUSION_EXPRESSIONS
       -DNDEBUG -DIPPP_MOCK_ANDROID_SDK
       -Iandroid/fusion -Iandroid/sdk-shim -Iandroid/runtime -Iandroid/tests/mock-sdk -I.
       -Wall -Wextra -Wno-unused-parameter)

"$CXX" "${FLAGS[@]}" \
    Runtime.cpp ACEs.cpp android/runtime/AndroidRuntime.cpp \
    android/jni/IniPlusPlusExtension.cc android/fusion/lSDK/UnicodeUtilities.cpp \
    -o "$BUILD/CRunINI++.so" -pthread

echo "built $BUILD/CRunINI++.so"

failures=0
symbols="$(nm -D --defined-only "$BUILD/CRunINI++.so")"
for symbol in extInit createRunObject getNumberOfConditions destroyRunObject handleRunObject action condition expression; do
	for prefix in 'CRunINI++' 'CRunIniPlusPlus'; do
		if ! grep -q "[[:space:]]${prefix}_${symbol}\$" <<<"$symbols"; then
			echo "error: ${prefix}_${symbol} is not exported" >&2
			failures=$((failures + 1))
		fi
	done
done

if [ "$failures" != 0 ]; then
	echo "error: $failures missing export(s)" >&2
	exit 1
fi
echo "all 16 entry points are exported"

# ... and that the dynamic linker resolves them (what the runtime's native loader does)
"$CXX" -std=c++20 -O1 android/tests/LoadProbe.cpp -o "$BUILD/load-probe" -ldl
"$BUILD/load-probe" "$BUILD/CRunINI++.so"
