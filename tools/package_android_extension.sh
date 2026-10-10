#!/usr/bin/env bash
# Build the Fusion Android extension ZIP using the runtime-merge layout: src/ and assets/ at root.
# The ZIP carries project-owned Java sources and native .so files only; it does not carry a Clickteam
# runtime/exporter, precompiled JAR/AAR, or AndroidX dependency. See docs/BUILD.md.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ANDROID_DIR="$ROOT/build/android"
OUT_DIR="$ROOT/build/android-package"
INSTALL_TO=""
ALLOW_INCOMPLETE=0
REPLACE_INSTALLED=0

while [ $# -gt 0 ]; do
	case "$1" in
		--android-dir) ANDROID_DIR="${2:?--android-dir needs a build_android.sh output directory}"; shift 2 ;;
		--android-dir=*) ANDROID_DIR="${1#*=}"; shift ;;
		--out) OUT_DIR="${2:?--out needs a directory}"; shift 2 ;;
		--out=*) OUT_DIR="${1#*=}"; shift ;;
		--install-to) INSTALL_TO="${2:?--install-to needs the Fusion Data/Runtime/Android directory}"; shift 2 ;;
		--install-to=*) INSTALL_TO="${1#*=}"; shift ;;
		--replace-installed) REPLACE_INSTALLED=1; shift ;;
		--allow-incomplete) ALLOW_INCOMPLETE=1; shift ;;
		-h|--help)
			cat <<'HELP'
Usage: tools/package_android_extension.sh [options]
  --android-dir DIR       output tree from tools/build_android.sh (default: build/android)
  --out DIR               ZIP output directory (default: build/android-package)
  --install-to DIR        install INI++.zip into Fusion's Data/Runtime/Android directory
  --replace-installed     allow replacing an existing INI++.zip (make a backup first)
  --allow-incomplete      create a marked source-only ZIP; it cannot be installed
HELP
			exit 0
			;;
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
if [ -n "$INSTALL_TO" ] && [ "${#MISSING[@]}" -ne 0 ]; then
	echo "error: refusing to install an incomplete source-only ZIP into Fusion's runtime directory" >&2
	exit 1
fi
if ! command -v zip >/dev/null 2>&1 || ! command -v unzip >/dev/null 2>&1; then
	echo "error: zip and unzip utilities are required to create and verify INI++.zip" >&2
	exit 1
fi

mkdir -p "$OUT_DIR"
OUT_DIR="$(cd "$OUT_DIR" && pwd)"
STAGE="$OUT_DIR/stage"
rm -rf "$STAGE"
mkdir -p "$STAGE/src/Extensions" "$STAGE/assets/mmf"
cp "$ROOT/android/java/Extensions/CRunIniPlusPlus.java" "$STAGE/src/Extensions/"
cp "$ROOT/android/java/Extensions/IniPlusPlusAceParams.java" "$STAGE/src/Extensions/"

for abi in "${EXPECTED_ABIS[@]}"; do
	library="$ANDROID_DIR/assets/mmf/$abi/libIniPlusPlusBridge.so"
	if [ -f "$library" ]; then
		mkdir -p "$STAGE/assets/mmf/$abi"
		cp "$library" "$STAGE/assets/mmf/$abi/"
	fi
done

cat > "$STAGE/README.txt" <<'EOF'
Ini++ Android extension source package

Install this ZIP as:
  <Fusion 2.5>/Data/Runtime/Android/INI++.zip

Archive layout follows the Clickteam Android extension merge convention:
  src/Extensions/CRunIniPlusPlus.java
  src/Extensions/IniPlusPlusAceParams.java
  assets/mmf/<ABI>/libIniPlusPlusBridge.so

The Java adapter is written against the Fusion 2.5 Java CRunExtension API surface present in the
Fusion 293.0 Android runtime sources and is intended to be compiled as source by the target Fusion
295.10 exporter. It contains no precompiled .class, .jar or .aar files and has no direct AndroidX
imports. The exporter supplies its own AndroidX/Gradle/SDK configuration; none is copied here.

The .so files are project-owned JNI libraries built by Android NDK r26.1.10909125. They are not
Clickteam RuntimeNative extensions and do not replace or imitate Clickteam's proprietary native ABI.
The root AndroidSDK_Gradle.zip is not included.

IMPORTANT: this source-package route has not yet been merged, compiled, exported or device-tested by
Fusion 295.10 in this repository. Confirm that the target exporter registers CRunIniPlusPlus in its
CExtLoad.java flow. Use the separate CExtLoad-registration.patch produced next to this ZIP as a
manual reference only; do not replace the target exporter's CExtLoad.java with another version.
Do not claim Android support complete until the unchanged MFA is exported and tested on a device.
EOF

# Keep a small manual registration reference outside the runtime merge ZIP; it is not a complete
# CExtLoad.java and therefore cannot overwrite the target exporter's vendor-owned loader.
cat > "$OUT_DIR/CExtLoad-registration.patch" <<'PATCH'
--- a/src/Extensions/CExtLoad.java
+++ b/src/Extensions/CExtLoad.java
@@
-        else if (name.compareToIgnoreCase("Android")==0)
+        else if (name.compareToIgnoreCase("INI++")==0 || name.compareToIgnoreCase("Ini++_Unicode")==0)
+        {
+            object=new CRunIniPlusPlus();
+        }
+        else if (name.compareToIgnoreCase("Android")==0)
PATCH

if [ "${#MISSING[@]}" -ne 0 ]; then
	{
		echo "INCOMPLETE: this ZIP has Java sources only; it cannot run until every configured ABI library is added."
		printf 'Missing: %s\n' "${MISSING[@]}"
	} > "$STAGE/MISSING-BINARIES.txt"
	ZIP_NAME="INI++-INCOMPLETE.zip"
else
	ZIP_NAME="INI++.zip"
fi

rm -f "$OUT_DIR/$ZIP_NAME"
ZIP_FILES=(src assets README.txt)
if [ "${#MISSING[@]}" -ne 0 ]; then
	ZIP_FILES+=(MISSING-BINARIES.txt)
fi
(cd "$STAGE" && zip -r -9 -q "$OUT_DIR/$ZIP_NAME" "${ZIP_FILES[@]}")

# Enforce the no-precompiled-Java-dependency requirement and verify the expected Fusion merge paths.
ENTRIES="$(unzip -Z1 "$OUT_DIR/$ZIP_NAME")"
for required in src/Extensions/CRunIniPlusPlus.java src/Extensions/IniPlusPlusAceParams.java; do
	if ! grep -Fxq "$required" <<<"$ENTRIES"; then
		echo "error: extension ZIP is missing required source entry: $required" >&2
		exit 1
	fi
done
if grep -Eiq '\.(class|jar|aar)$|(^|/)AndroidSDK_Gradle\.zip$' <<<"$ENTRIES"; then
	echo "error: refusing to package compiled Java or proprietary exporter artifacts" >&2
	exit 1
fi
if [ "${#MISSING[@]}" -eq 0 ]; then
	for abi in "${EXPECTED_ABIS[@]}"; do
		if ! grep -Fxq "assets/mmf/$abi/libIniPlusPlusBridge.so" <<<"$ENTRIES"; then
			echo "error: extension ZIP is missing JNI library for ABI $abi" >&2
			exit 1
		fi
	done
fi

echo "created $OUT_DIR/$ZIP_NAME"
unzip -l "$OUT_DIR/$ZIP_NAME"
if [ -n "$INSTALL_TO" ]; then
	mkdir -p "$INSTALL_TO"
	DEST="$INSTALL_TO/INI++.zip"
	if [ -e "$DEST" ] && [ "$REPLACE_INSTALLED" = 0 ]; then
		echo "error: refusing to overwrite $DEST; pass --replace-installed after making a backup" >&2
		exit 1
	fi
	cp -f "$OUT_DIR/$ZIP_NAME" "$DEST"
	echo "installed Fusion Android extension ZIP: $DEST"
fi
