#!/usr/bin/env bash
# Build the Ini++ Android runtime extension (ndk-build).
#
# Usage:
#   tools/build_android.sh [--sdk-dir DIR] [--ndk DIR] [--abis "arm64-v8a armeabi-v7a x86_64"]
#                          [--out DIR] [--check-only]
#
# This native C++ ABI requires Clickteam's <RuntimeNative.h>. The current Gradle archive checked
# for this project does not contain it, and no complete, current public native-C++ SDK has been
# verified. Do not substitute the repo's mock/shim. Ask Clickteam for a compatible, licensed SDK;
# if supplied, point to it with --sdk-dir / $IPPP_ANDROID_SDK_DIR. See docs/BUILD.md section 2.
# Without the real header this build stops with an explanation.
#
# On success the .so files are copied to <out>/<abi>/ as both CRunINI++.so (the name the runtime
# looks for with INI++.mfx) and CRunIniPlusPlus.so (for MFX files renamed to a valid identifier).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

SDK_DIR="${IPPP_ANDROID_SDK_DIR:-}"
NDK_DIR="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
ABIS=""
OUT_DIR="$REPO_ROOT/build/android"
CHECK_ONLY=0
NDK_BUILD_ARGS=()

while [ $# -gt 0 ]; do
	case "$1" in
		--sdk-dir) SDK_DIR="${2:?--sdk-dir needs a directory}"; shift 2 ;;
		--sdk-dir=*) SDK_DIR="${1#*=}"; shift ;;
		--ndk) NDK_DIR="${2:?--ndk needs a directory}"; shift 2 ;;
		--ndk=*) NDK_DIR="${1#*=}"; shift ;;
		--abis) ABIS="${2:?--abis needs a value}"; shift 2 ;;
		--abis=*) ABIS="${1#*=}"; shift ;;
		--out) OUT_DIR="${2:?--out needs a directory}"; shift 2 ;;
		--out=*) OUT_DIR="${1#*=}"; shift ;;
		--check-only) CHECK_ONLY=1; shift ;;
		-h|--help) sed -n '2,20p' "$0"; exit 0 ;;
		*) echo "unknown argument: $1" >&2; exit 2 ;;
	esac
done

# ---------------------------------------------------------------------------------------------
# ACE tables must match the ACE declarations (Menus.cpp)
# ---------------------------------------------------------------------------------------------
if command -v python3 >/dev/null 2>&1; then
	if [ "$CHECK_ONLY" = 1 ]; then
		python3 tools/gen_android_aces.py --check
		exit 0
	fi
	python3 tools/gen_android_aces.py
else
	echo "warning: python3 not found; using the checked-in android/runtime/AceDispatch.inc as-is" >&2
	if [ "$CHECK_ONLY" = 1 ]; then
		echo "error: --check-only needs python3" >&2
		exit 1
	fi
fi

if [ "$CHECK_ONLY" = 0 ]; then
	# ndk-build can come from PATH or from an NDK installation
	if [ -z "$NDK_DIR" ]; then
		if command -v ndk-build >/dev/null 2>&1; then
			NDK_BUILD="$(command -v ndk-build)"
		else
			cat >&2 <<'EOF'
error: ndk-build was not found.

Install the Android NDK (r23 or newer: the implementation needs a C++20 compiler) and either put
ndk-build on PATH, or pass --ndk /path/to/ndk, or set ANDROID_NDK_HOME.  See docs/BUILD.md for the
exact versions this project is tested with.
EOF
			exit 1
		fi
	else
		if [ -x "$NDK_DIR/ndk-build" ]; then
			NDK_BUILD="$NDK_DIR/ndk-build"
		elif [ -x "$NDK_DIR/ndk-build.sh" ]; then
			NDK_BUILD="$NDK_DIR/ndk-build.sh"
		else
			echo "error: $NDK_DIR does not look like an Android NDK (no ndk-build inside)." >&2
			exit 1
		fi
		NDK_TOOLCHAIN_BIN="$(find "$NDK_DIR/toolchains/llvm/prebuilt" -maxdepth 2 -name 'llvm-nm' -print -quit 2>/dev/null || true)"
	fi
fi

# ---------------------------------------------------------------------------------------------
# The proprietary SDK header. The extension needs the exact RuntimeNative.h (and its includes).
# If an authorized compatible SDK is provided, probe common legacy/current layouts; this does not
# imply that such a complete SDK is presently available.
# ---------------------------------------------------------------------------------------------
find_sdk_inc() {
	local dir="$1"
	local candidate
	# The layouts seen in the wild: the header alone in a folder, an inc/include subfolder, the
	# official SDK's android/jni/ (native/base/NativeExtension.h includes ../../android/jni/...),
	# and NDK project trees (jni/, native/include/, ...).
	for candidate in "$dir" "$dir/inc" "$dir/Inc" "$dir/include" "$dir/Include" \
	                 "$dir/jni" "$dir/android/jni" "$dir/android/inc" "$dir/android/Inc" \
	                 "$dir/android/include" "$dir/native/inc" "$dir/native/include" \
	                 "$dir/native/jni" "$dir/sdk/inc" "$dir/sdk/Inc" "$dir/sdk/include" \
	                 "$dir/inc/android" "$dir/include/android"; do
		if [ -f "$candidate/RuntimeNative.h" ]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done
	# Anything else: look for the header, but not in a whole filesystem tree.
	local found
	found="$(find "$dir" -maxdepth 6 -name 'RuntimeNative.h' -print -quit 2>/dev/null || true)"
	if [ -n "$found" ]; then
		dirname "$found"
		return 0
	fi
	return 1
}

SDK_INC=""
if [ -n "$SDK_DIR" ]; then
	if ! SDK_INC="$(find_sdk_inc "$SDK_DIR")"; then
		cat >&2 <<EOF
error: RuntimeNative.h was not found under "$SDK_DIR".

Expected it in the SDK directory itself or in one of: inc/, Inc/, include/, Include/, jni/,
android/jni/, android/inc/, android/include/, native/include/, sdk/inc/ (or anywhere below
"$SDK_DIR" within six levels).

The supplied directory has no RuntimeNative.h. The currently available Gradle archive checked for
this project does not contain the legacy native C++ header; no complete, current public SDK has been
verified. See docs/BUILD.md, section 2, and ask Clickteam to confirm/provide a compatible, licensed
C++/NDK extension SDK. Do not use the repository's mock or shim for a distributable extension.
EOF
		exit 1
	fi
else
	cat >&2 <<'EOF'
error: the Clickteam Android extension SDK was not provided, so <RuntimeNative.h> is missing.

This header cannot be replaced: it declares the RuntimeFunctions interface the Android runtime
calls the extension through, and every Android extension is compiled against it.

This is not supplied by Android Studio, Android SDK/NDK platform packages, or Gradle. The current
Gradle archive checked for this project does not contain the required header, and a compatible,
complete public Clickteam C++ SDK has not been verified. See docs/BUILD.md, section 2, and ask
Clickteam support/forum specifically for the licensed native C++/NDK extension SDK compatible with
your Fusion/Android Exporter build. Do not use a reconstructed/mock header.

If Clickteam provides the compatible SDK, then:
  * locally, run tools/build_android.sh --sdk-dir /path/to/extracted/sdk
    or set IPPP_ANDROID_SDK_DIR=/path/to/extracted/sdk
  * in CI, use CLICKTEAM_ANDROID_SDK_B64 (tools/make_sdk_b64.sh) or a self-hosted runner's
    IPPP_ANDROID_SDK_DIR. These only transport a header you already have; they cannot create it.

android/tests/mock-sdk/RuntimeNative.h is a stand-in used by the host tests (tools/run_host_tests.sh)
only; it must not be used for a distributable extension.
EOF
	exit 1
fi

echo "Android SDK include directory: $SDK_INC"

# Reject the stand-ins even if they were pointed at explicitly.
if grep -q "IPPP_MOCK_RUNTIME_NATIVE_H" "$SDK_INC/RuntimeNative.h"; then
	echo "error: $SDK_INC/RuntimeNative.h is android/tests/mock-sdk/RuntimeNative.h, the host-test stand-in, not the Clickteam SDK header." >&2
	exit 1
fi
if grep -q "RuntimeNative_Shim_HeaderPlusPlus" "$SDK_INC/RuntimeNative.h"; then
	echo "error: $SDK_INC/RuntimeNative.h is android/sdk-shim/RuntimeNative.h, the documented reconstruction, not the Clickteam SDK header." >&2
	exit 1
fi

# ---------------------------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------------------------
if [ -n "$ABIS" ]; then
	NDK_BUILD_ARGS+=("APP_ABI=$ABIS")
fi

echo "ndk-build: $NDK_BUILD"
"$NDK_BUILD" -C android/jni \
	NDK_PROJECT_PATH=. \
	APP_BUILD_SCRIPT=Android.mk \
	NDK_APPLICATION_MK=Application.mk \
	IPPP_ANDROID_SDK_INC="$SDK_INC" \
	NDK_OUT="$REPO_ROOT/build/android/obj" \
	NDK_LIBS_OUT="$REPO_ROOT/build/android/libs" \
	"${NDK_BUILD_ARGS[@]}"

# ---------------------------------------------------------------------------------------------
# Package + verify the exported entry points
# ---------------------------------------------------------------------------------------------
NM="$(command -v llvm-nm || command -v nm || true)"
if [ -z "$NM" ] && [ -n "${NDK_TOOLCHAIN_BIN:-}" ]; then
	NM="$NDK_TOOLCHAIN_BIN"
fi

required_symbols=(
	CRunINI++_extInit
	CRunINI++_createRunObject
	CRunINI++_getNumberOfConditions
	CRunINI++_destroyRunObject
	CRunINI++_handleRunObject
	CRunINI++_action
	CRunINI++_condition
	CRunINI++_expression
)

failures=0
for abi_dir in "$REPO_ROOT/build/android/libs"/*; do
	[ -d "$abi_dir" ] || continue
	abi="$(basename "$abi_dir")"
	src="$abi_dir/libCRunIniPlusPlus.so"
	if [ ! -f "$src" ]; then
		echo "error: $src was not produced" >&2
		failures=$((failures + 1))
		continue
	fi
	mkdir -p "$OUT_DIR/$abi"
	cp -f "$src" "$OUT_DIR/$abi/CRunIniPlusPlus.so"
	cp -f "$src" "$OUT_DIR/$abi/CRunINI++.so"

	if [ -n "$NM" ]; then
		symbols="$("$NM" -D --defined-only "$src" 2>/dev/null || "$NM" -D "$src" 2>/dev/null || true)"
		for symbol in "${required_symbols[@]}"; do
			if ! grep -q "[[:space:]]${symbol}$" <<<"$symbols"; then
				echo "error: $abi/CRunINI++.so does not export ${symbol}" >&2
				failures=$((failures + 1))
			fi
		done
		echo "verified ${abi}/CRunINI++.so ($(du -h "$src" | cut -f1))"
	else
		echo "warning: no nm available, skipping the exported-symbol check for $abi" >&2
	fi
done

if [ "$failures" != 0 ]; then
	echo "error: $failures problem(s) while packaging the Android extension" >&2
	exit 1
fi

cat <<EOF

Android extension built:
$(cd "$OUT_DIR" && ls -1 */*.so | sed 's/^/  /')

Copy <out>/<abi>/CRunINI++.so into the APK build's assets/mmf/<abi>/ folder (or configure the
extension's natives for the exporter; see docs/INSTALL.md).
EOF
