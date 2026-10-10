#!/usr/bin/env bash
# Assemble the project-owned Android Java/JNI overlay as an INI++.zip.
#
# This ZIP contains no Clickteam SDK/exporter files and is not a tested out-of-the-box installer.
# Its app/ tree is for the extracted Fusion 2.5 Gradle project after verifying build 295.10 with
# tools/integrate_android_exporter.sh. All expected ABI libraries must be built by tools/build_android.sh.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ANDROID_DIR="$ROOT/build/android"
OUT_DIR="$ROOT/build/android-package"
ALLOW_INCOMPLETE=0

while [ $# -gt 0 ]; do
	case "$1" in
		--android-dir) ANDROID_DIR="${2:?--android-dir needs a build_android.sh output directory}"; shift 2 ;;
		--android-dir=*) ANDROID_DIR="${1#*=}"; shift ;;
		--out) OUT_DIR="${2:?--out needs a directory}"; shift 2 ;;
		--out=*) OUT_DIR="${1#*=}"; shift ;;
		--allow-incomplete) ALLOW_INCOMPLETE=1; shift ;;
		-h|--help) sed -n '2,11p' "$0"; exit 0 ;;
		*) echo "unknown argument: $1" >&2; exit 2 ;;
	esac
done

EXPECTED_ABIS=(arm64-v8a armeabi-v7a x86_64)
MISSING=()
for abi in "${EXPECTED_ABIS[@]}"; do
	if [ ! -f "$ANDROID_DIR/assets/mmf/$abi/libIniPlusPlusBridge.so" ]; then
		MISSING+=("$abi/libIniPlusPlusBridge.so")
	fi
done

if [ "${#MISSING[@]}" -ne 0 ] && [ "$ALLOW_INCOMPLETE" = 0 ]; then
	printf 'error: refusing to make INI++.zip; missing NDK runtime libraries:\n' >&2
	printf '  - %s\n' "${MISSING[@]}" >&2
	printf 'Run tools/build_android.sh with NDK r26.1.10909125, or use --allow-incomplete only for a clearly marked source bundle.\n' >&2
	exit 1
fi
if ! command -v zip >/dev/null 2>&1; then
	echo "error: the zip utility is required to create INI++.zip" >&2
	exit 1
fi

mkdir -p "$OUT_DIR"
OUT_DIR="$(cd "$OUT_DIR" && pwd)"
STAGE="$OUT_DIR/stage"
rm -rf "$STAGE"
mkdir -p "$STAGE/app/src/main/java/Extensions" "$STAGE/app/src/main/assets/mmf" "$STAGE/integration"
cp "$ROOT/android/java/Extensions/CRunIniPlusPlus.java" "$STAGE/app/src/main/java/Extensions/"
cp "$ROOT/android/java/Extensions/IniPlusPlusAceParams.java" "$STAGE/app/src/main/java/Extensions/"

for abi in "${EXPECTED_ABIS[@]}"; do
	library="$ANDROID_DIR/assets/mmf/$abi/libIniPlusPlusBridge.so"
	if [ -f "$library" ]; then
		mkdir -p "$STAGE/app/src/main/assets/mmf/$abi"
		cp "$library" "$STAGE/app/src/main/assets/mmf/$abi/"
	fi
done

cat > "$STAGE/integration/CExtLoad-registration.patch" <<'PATCH'
--- a/app/src/main/java/Extensions/CExtLoad.java
+++ b/app/src/main/java/Extensions/CExtLoad.java
@@
-        else if (name.compareToIgnoreCase("Android")==0)
+        else if (name.compareToIgnoreCase("INI++")==0 || name.compareToIgnoreCase("Ini++_Unicode")==0)
+        {
+            object=new CRunIniPlusPlus();
+        }
+        else if (name.compareToIgnoreCase("Android")==0)
PATCH

cat > "$STAGE/README.txt" <<'EOF'
Ini++ Android Java/JNI overlay for the Fusion 2.5 Gradle exporter

This overlay contains Ini++-owned Java adapter sources and (when present) the NDK JNI library for
Fusion's mmf/<ABI> assets. It deliberately excludes all proprietary Clickteam SDK/exporter files.
It requires the exact Fusion 2.5 build 295.10 Gradle exporter source tree and a working NDK build.

IMPORTANT: This source bundle has not been compiled against the exact 295.10 exporter, merged by a
Fusion export, used to export the Sonic MFA, or run on a device. It is not a verified release. The
repository's currently available AndroidSDK_Gradle.zip reports Fusion 292.0, so the integration
script refuses it rather than silently using an older runtime.

For integration, use the repository script:
  tools/integrate_android_exporter.sh --exporter <authorized-295.10-project> --android-dir <ndk-output>
Then build with that project's Gradle wrapper and export/test a normal MFA. See docs/BUILD.md and
docs/INSTALL.md. Do not install the overlay without the CExtLoad Java registration.
EOF

if [ "${#MISSING[@]}" -ne 0 ]; then
	{
		echo "INCOMPLETE: this ZIP is a source overlay only; required NDK libraries are missing."
		printf 'Missing: %s\n' "${MISSING[@]}"
	} > "$STAGE/MISSING-BINARIES.txt"
	ZIP_NAME="INI++-INCOMPLETE.zip"
else
	ZIP_NAME="INI++.zip"
fi

rm -f "$OUT_DIR/$ZIP_NAME"
ZIP_FILES=(app integration README.txt)
if [ "${#MISSING[@]}" -ne 0 ]; then
	ZIP_FILES+=(MISSING-BINARIES.txt)
fi
(cd "$STAGE" && zip -r -9 -q "$OUT_DIR/$ZIP_NAME" "${ZIP_FILES[@]}")

echo "created $OUT_DIR/$ZIP_NAME"
unzip -l "$OUT_DIR/$ZIP_NAME"
