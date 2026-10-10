#!/usr/bin/env bash
# Smoke-test the Data/Runtime/Android ZIP layout using tiny placeholder .so files.
# This checks packaging only; placeholder libraries are never a usable Android build artifact.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/iniplusplus-android-package.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

ANDROID_DIR="$TMP/native"
for abi in arm64-v8a armeabi-v7a x86_64; do
	mkdir -p "$ANDROID_DIR/assets/mmf/$abi"
	printf 'test placeholder, not an ELF library: %s\n' "$abi" \
		> "$ANDROID_DIR/assets/mmf/$abi/libIniPlusPlusBridge.so"
done

FUSION_ANDROID="$TMP/Fusion 2.5/Data/Runtime/Android"
OUT="$TMP/package"
bash "$ROOT/tools/package_android_extension.sh" \
	--android-dir "$ANDROID_DIR" \
	--out "$OUT" \
	--install-to "$FUSION_ANDROID" >/dev/null

ZIP="$FUSION_ANDROID/INI++.zip"
[ -f "$ZIP" ] || { echo "error: package was not installed to Data/Runtime/Android" >&2; exit 1; }
if bash "$ROOT/tools/package_android_extension.sh" \
	--android-dir "$ANDROID_DIR" --out "$OUT" --install-to "$FUSION_ANDROID" >/dev/null 2>&1; then
	echo "error: installer should refuse to overwrite an existing extension ZIP by default" >&2
	exit 1
fi
bash "$ROOT/tools/package_android_extension.sh" \
	--android-dir "$ANDROID_DIR" --out "$OUT" --install-to "$FUSION_ANDROID" --replace-installed >/dev/null
ENTRIES="$(unzip -Z1 "$ZIP")"
for required in \
	src/Extensions/CRunIniPlusPlus.java \
	src/Extensions/IniPlusPlusAceParams.java \
	assets/mmf/arm64-v8a/libIniPlusPlusBridge.so \
	assets/mmf/armeabi-v7a/libIniPlusPlusBridge.so \
	assets/mmf/x86_64/libIniPlusPlusBridge.so; do
	grep -Fxq "$required" <<<"$ENTRIES" || {
		echo "error: package is missing $required" >&2
		exit 1
	}
done
if grep -Eiq '\.(class|jar|aar)$|AndroidSDK_Gradle\.zip|app/src/main/' <<<"$ENTRIES"; then
	echo "error: package contains compiled Java/proprietary exporter files or an incorrect project-overlay path" >&2
	exit 1
fi
if ! grep -Fq 'Fusion 293.0' <(unzip -p "$ZIP" README.txt); then
	echo "error: package README does not record the Java API baseline" >&2
	exit 1
fi
if grep -R -n -E '^[[:space:]]*import[[:space:]]+androidx\.' "$ROOT/android/java/Extensions"; then
	echo "error: project Java sources must not have direct AndroidX imports/dependencies" >&2
	exit 1
fi

INCOMPLETE_OUT="$TMP/incomplete"
if bash "$ROOT/tools/package_android_extension.sh" \
	--android-dir "$TMP/empty-native" --out "$INCOMPLETE_OUT" >/dev/null 2>&1; then
	echo "error: packager should reject missing ABI libraries by default" >&2
	exit 1
fi
bash "$ROOT/tools/package_android_extension.sh" \
	--android-dir "$TMP/empty-native" --out "$INCOMPLETE_OUT" --allow-incomplete >/dev/null
[ -f "$INCOMPLETE_OUT/INI++-INCOMPLETE.zip" ] || {
	echo "error: marked incomplete ZIP was not created for source-layout debugging" >&2
	exit 1
}
if bash "$ROOT/tools/package_android_extension.sh" \
	--android-dir "$TMP/empty-native" --out "$INCOMPLETE_OUT" \
	--allow-incomplete --install-to "$TMP/should-not-install" >/dev/null 2>&1; then
	echo "error: packager should refuse to install an incomplete ZIP" >&2
	exit 1
fi

echo "Android extension ZIP packaging test passed (source + ABI assets, no JAR/AAR, correct merge layout and incomplete-package guard)."
