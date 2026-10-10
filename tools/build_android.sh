#!/usr/bin/env bash
# Reproducibly build the project-owned JNI runtime library with Android NDK ndk-build.
# This does not need RuntimeNative.h: package the resulting Java sources and ABI libraries into a
# Fusion Data/Runtime/Android extension ZIP with tools/package_android_extension.sh.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

NDK_DIR="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
ABIS=""
OUT_DIR="$ROOT/build/android"
CHECK_ONLY=0
CLEAN=0

while [ $# -gt 0 ]; do
	case "$1" in
		--ndk) NDK_DIR="${2:?--ndk needs a directory}"; shift 2 ;;
		--ndk=*) NDK_DIR="${1#*=}"; shift ;;
		--abis) ABIS="${2:?--abis needs a value}"; shift 2 ;;
		--abis=*) ABIS="${1#*=}"; shift ;;
		--out) OUT_DIR="${2:?--out needs a directory}"; shift 2 ;;
		--out=*) OUT_DIR="${1#*=}"; shift ;;
		--check-only) CHECK_ONLY=1; shift ;;
		--clean) CLEAN=1; shift ;;
		-h|--help)
			sed -n '2,9p' "$0"
			exit 0
			;;
		*) echo "unknown argument: $1" >&2; exit 2 ;;
	esac
done

python3 tools/gen_android_aces.py --check
if [ "$CHECK_ONLY" = 1 ]; then
	echo "ACE metadata is up to date. NDK compile was not requested (--check-only)."
	exit 0
fi

if [ -z "$NDK_DIR" ]; then
	NDK_BUILD="$(command -v ndk-build || true)"
else
	NDK_BUILD=""
	for candidate in "$NDK_DIR/ndk-build" "$NDK_DIR/ndk-build.sh"; do
		if [ -x "$candidate" ]; then
			NDK_BUILD="$candidate"
			break
		fi
	done
fi
if [ -z "$NDK_BUILD" ]; then
	cat >&2 <<'EOF'
error: Android NDK ndk-build was not found.
Install the pinned NDK (r26.1.10909125 in CI), then either:
  export ANDROID_NDK_HOME=/path/to/android-ndk-r26.1.10909125
  tools/build_android.sh
or pass --ndk /path/to/android-ndk-r26.1.10909125. No proprietary Clickteam C++ header is
needed for this Java/JNI route. A Java compile and APK export still require the matching Fusion
2.5 Android exporter; see docs/BUILD.md.
EOF
	exit 1
fi
if [ -z "$NDK_DIR" ]; then
	NDK_DIR="$(python3 -c 'import os,sys; print(os.path.dirname(os.path.realpath(sys.argv[1])))' "$NDK_BUILD")"
fi
NDK_VERSION="$(sed -n 's/^Pkg.Revision[[:space:]]*=[[:space:]]*//p' "$NDK_DIR/source.properties" 2>/dev/null | head -1)"
if [ "$NDK_VERSION" != "26.1.10909125" ]; then
	echo "error: expected Android NDK 26.1.10909125, found ${NDK_VERSION:-unknown} under $NDK_DIR" >&2
	echo "Install the pinned NDK or update the documented/CI toolchain together; refusing an unverified version." >&2
	exit 1
fi

mkdir -p "$OUT_DIR"
OUT_DIR="$(cd "$OUT_DIR" && pwd)"
if [ "$CLEAN" = 1 ]; then
	rm -rf "$OUT_DIR/obj" "$OUT_DIR/libs" "$OUT_DIR/assets"
fi

NDK_ARGS=(
	-C android/jni
	NDK_PROJECT_PATH=.
	APP_BUILD_SCRIPT=Android.mk
	NDK_APPLICATION_MK=Application.mk
	NDK_OUT="$OUT_DIR/obj"
	NDK_LIBS_OUT="$OUT_DIR/libs"
)
if [ -n "$ABIS" ]; then
	NDK_ARGS+=("APP_ABI=$ABIS")
fi
"$NDK_BUILD" "${NDK_ARGS[@]}" -j"${JOBS:-2}"

# ndk-build names the module libIniPlusPlusBridge.so. The Fusion Gradle runtime lists mmf/<abi>,
# loads non-CRun*.so assets with System.load(), and CRunIniPlusPlus resolves its JNI symbols.
for abi_dir in "$OUT_DIR/libs"/*; do
	[ -d "$abi_dir" ] || continue
	abi="$(basename "$abi_dir")"
	library="$abi_dir/libIniPlusPlusBridge.so"
	if [ ! -f "$library" ]; then
		echo "error: expected NDK output $library was not produced" >&2
		exit 1
	fi
	asset_dir="$OUT_DIR/assets/mmf/$abi"
	mkdir -p "$asset_dir"
	cp -f "$library" "$asset_dir/libIniPlusPlusBridge.so"

	NM="$(command -v llvm-nm || true)"
	if [ -z "$NM" ] && [ -n "$NDK_DIR" ]; then
		NM="$(find "$NDK_DIR/toolchains/llvm/prebuilt" -type f -name llvm-nm -print -quit 2>/dev/null || true)"
	fi
	if [ -z "$NM" ]; then
		echo "error: llvm-nm from the selected NDK is required to verify JNI exports" >&2
		exit 1
	fi
	symbols="$("$NM" --dynamic --defined-only "$library")"
	for symbol in \
		Java_Extensions_CRunIniPlusPlus_nativeGetNumberOfConditions \
		Java_Extensions_CRunIniPlusPlus_nativeCreate \
		Java_Extensions_CRunIniPlusPlus_nativeDestroy \
		Java_Extensions_CRunIniPlusPlus_nativeHandle \
		Java_Extensions_CRunIniPlusPlus_nativeAction \
		Java_Extensions_CRunIniPlusPlus_nativeCondition \
		Java_Extensions_CRunIniPlusPlus_nativeExpression; do
		if ! grep -Fq "$symbol" <<<"$symbols"; then
			echo "error: $abi/libIniPlusPlusBridge.so does not export $symbol" >&2
			exit 1
		fi
	done
	echo "verified $abi/libIniPlusPlusBridge.so ($(du -h "$library" | cut -f1))"
done

if ! compgen -G "$OUT_DIR/assets/mmf/*/libIniPlusPlusBridge.so" >/dev/null; then
	echo "error: ndk-build did not create any ABI libraries under $OUT_DIR/assets/mmf" >&2
	exit 1
fi

cat <<EOF

JNI runtime library build complete: $OUT_DIR/assets/mmf/<abi>/libIniPlusPlusBridge.so

Package the Java sources and ABI libraries in Clickteam's Android-extension merge layout:
  tools/package_android_extension.sh --android-dir "$OUT_DIR" --out build/android-package
Then install build/android-package/INI++.zip to <Fusion>/Data/Runtime/Android/INI++.zip.
The source adapter uses the Fusion 293.0 CRunExtension Java API surface; the target Fusion 295.10
exporter must still be allowed to merge, register and compile this package. No APK export/device
test is implied by this NDK build.
EOF
