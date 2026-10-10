#!/usr/bin/env bash
# Assemble a clearly labelled release candidate. This tool never labels an Android build verified:
# that requires the matching Fusion 295.10 exporter, a normal MFA/APK export, and a device test.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="$(git -C "$ROOT" describe --tags --always --dirty 2>/dev/null || echo dev)"
VERSION="${VERSION#v}"
WIN_RUNTIME=""
WIN_EDITTIME=""
ANDROID_ZIP=""
OUT_DIR="$ROOT/build/release"
WITH_SOURCE=0

while [ $# -gt 0 ]; do
	case "$1" in
		--windows-runtime) WIN_RUNTIME="${2:?--windows-runtime needs a built runtime MFX}"; shift 2 ;;
		--windows-runtime=*) WIN_RUNTIME="${1#*=}"; shift ;;
		--windows-edittime) WIN_EDITTIME="${2:?--windows-edittime needs a built edittime MFX}"; shift 2 ;;
		--windows-edittime=*) WIN_EDITTIME="${1#*=}"; shift ;;
		--android-zip) ANDROID_ZIP="${2:?--android-zip needs a built INI++.zip}"; shift 2 ;;
		--android-zip=*) ANDROID_ZIP="${1#*=}"; shift ;;
		--version) VERSION="${2:?--version needs a version string}"; shift 2 ;;
		--version=*) VERSION="${1#*=}"; shift ;;
		--out) OUT_DIR="${2:?--out needs a directory}"; shift 2 ;;
		--out=*) OUT_DIR="${1#*=}"; shift ;;
		--source) WITH_SOURCE=1; shift ;;
		-h|--help) sed -n '2,11p' "$0"; exit 0 ;;
		*) echo "unknown argument: $1" >&2; exit 2 ;;
	esac
done

for file in "$WIN_RUNTIME" "$WIN_EDITTIME" "$ANDROID_ZIP"; do
	if [ -z "$file" ] || [ ! -f "$file" ]; then
		echo "error: missing required build input: ${file:-<not supplied>}" >&2
		echo "Build both Windows MFX files and tools/package_android_extension.sh output first." >&2
		exit 1
	fi
done
if [[ "$(basename "$ANDROID_ZIP")" == *INCOMPLETE* ]]; then
	echo "error: refusing to include a source-only/incomplete Android ZIP in a release candidate" >&2
	exit 1
fi
command -v zip >/dev/null 2>&1 || { echo "error: zip is required" >&2; exit 1; }

VERSION="$(printf '%s' "$VERSION" | tr -c 'A-Za-z0-9._-' '-')"
NAME="IniPlusPlus-$VERSION-candidate"
STAGE="$OUT_DIR/$NAME"
mkdir -p "$OUT_DIR"
rm -rf "$STAGE"
mkdir -p "$STAGE/windows/runtime" "$STAGE/windows/edittime" "$STAGE/android" "$STAGE/docs"
cp "$WIN_RUNTIME" "$STAGE/windows/runtime/INI++.mfx"
cp "$WIN_EDITTIME" "$STAGE/windows/edittime/INI++.mfx"
cp "$ANDROID_ZIP" "$STAGE/android/INI++.zip"
cp "$ROOT/docs/BUILD.md" "$ROOT/docs/INSTALL.md" "$ROOT/docs/COMPATIBILITY.md" "$STAGE/docs/"
cp "$ROOT/packaging/install-windows.bat" "$STAGE/windows/install.bat"

cat > "$STAGE/ANDROID-STATUS.txt" <<'EOF'
ANDROID STATUS: UNVERIFIED RELEASE CANDIDATE

The Android integration has not yet been compiled against the exact Fusion 2.5 build 295.10
Gradle exporter, merged through a Fusion export, used to export the Sonic MFA, or tested on a
physical device. Do not treat this candidate as a verified Android release.

The repository's AndroidSDK_Gradle.zip currently reports runtime version Fusion 292.0, and the
integration helper rejects that mismatch instead of silently substituting it. Obtain the authorized
295.10 exporter project, run tools/integrate_android_exporter.sh, export the user's unmodified MFA,
and complete the device/data-compatibility checks before marking Android support as complete.
EOF

if [ "$WITH_SOURCE" = 1 ]; then
	mkdir -p "$STAGE/source"
	# Build from tracked project files but deliberately exclude the root proprietary exporter ZIP.
	git -C "$ROOT" ls-files -z | tar -C "$ROOT" -czf "$STAGE/source/IniPlusPlus-$VERSION-source.tar.gz" \
		--exclude=AndroidSDK_Gradle.zip --null -T -
fi

ZIP="$OUT_DIR/$NAME.zip"
rm -f "$ZIP"
(cd "$OUT_DIR" && zip -r -9 -q "$NAME.zip" "$NAME")
echo "created unverified release candidate: $ZIP"
echo "This file is not a completed Android release; see $STAGE/ANDROID-STATUS.txt"
