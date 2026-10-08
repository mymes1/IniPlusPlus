#!/usr/bin/env bash
# Assembles the installable Ini++ release for Clickteam Fusion 2.5:
#
#   IniPlusPlus-<version>.zip
#     README.txt
#     windows/install.bat            one-click installation (or manual copy, see docs/INSTALL.md)
#     windows/edittime/INI++.mfx     edittime MFX
#     windows/runtime/INI++.mfx      runtime MFX
#     android/INI++.zip              Fusion Android extension package (Data/Runtime/Android/)
#     android/assets/mmf/<abi>/CRunINI++.so
#     docs/{INSTALL,COMPATIBILITY,BUILD}.md
#
# The two binary sets have to come from real builds - a Windows machine with MSVC for the MFX and
# the NDK + the Clickteam Android SDK for the .so files (see docs/BUILD.md; CI does both and calls
# this script in its "package" job).  Without them the script fails and says exactly what is
# missing, unless --allow-missing is given (which adds a MISSING-BINARIES.txt marker to the zip so
# it cannot be mistaken for a complete release).
#
# Usage:
#   tools/package_release.sh --windows-runtime <INI++.mfx> --windows-edittime <INI++.mfx> \
#                            --android-dir <dir> [--version <v>] [--out <dir>] [--source]
#   tools/package_release.sh --allow-missing            # layout/smoke test without binaries
#
# <dir> for --android-dir is a directory containing either
#   <abi>/CRunINI++.so                       (tools/build_android.sh --out <dir>)
#   assets/mmf/<abi>/CRunINI++.so            (a package folder, or an unpacked APK)
# and optionally INI++.zip (the ready-made Fusion package).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

VERSION=""
OUT_DIR="$REPO_ROOT/build/release"
WIN_RUNTIME=""
WIN_EDITTIME=""
ANDROID_DIR=""
ALLOW_MISSING=0
WITH_SOURCE=0

while [ $# -gt 0 ]; do
	case "$1" in
		--windows-runtime) WIN_RUNTIME="${2:?}"; shift 2 ;;
		--windows-runtime=*) WIN_RUNTIME="${1#*=}"; shift ;;
		--windows-edittime) WIN_EDITTIME="${2:?}"; shift 2 ;;
		--windows-edittime=*) WIN_EDITTIME="${1#*=}"; shift ;;
		--android-dir) ANDROID_DIR="${2:?}"; shift 2 ;;
		--android-dir=*) ANDROID_DIR="${1#*=}"; shift ;;
		--version) VERSION="${2:?}"; shift 2 ;;
		--version=*) VERSION="${1#*=}"; shift ;;
		--out) OUT_DIR="${2:?}"; shift 2 ;;
		--out=*) OUT_DIR="${1#*=}"; shift ;;
		--allow-missing) ALLOW_MISSING=1; shift ;;
		--source) WITH_SOURCE=1; shift ;;
		-h|--help) sed -n '2,32p' "$0"; exit 0 ;;
		*) echo "unknown argument: $1" >&2; exit 2 ;;
	esac
done

if [ -z "$VERSION" ]; then
	VERSION="$(git describe --tags --always --dirty 2>/dev/null || echo dev)"
fi
VERSION="${VERSION#v}"
VERSION="$(printf '%s' "$VERSION" | tr -c 'A-Za-z0-9._-' '-')"

NAME="IniPlusPlus-$VERSION"
STAGE="$OUT_DIR/$NAME"
ZIP="$OUT_DIR/$NAME.zip"
MISSING=()

echo "Ini++ release packaging"
echo "  version: $VERSION"
echo "  staging: $STAGE"

rm -rf "$STAGE"
mkdir -p "$STAGE/windows/edittime" "$STAGE/windows/runtime" "$STAGE/android/assets/mmf" "$STAGE/docs"

# -------------------------------------------------------------------------------------------
# Windows MFX
# -------------------------------------------------------------------------------------------
copy_mfx() {
	local src="$1" dest="$2" what="$3"
	if [ -z "$src" ]; then
		MISSING+=("$what (no path given)")
		return
	fi
	if [ ! -f "$src" ]; then
		MISSING+=("$what ($src does not exist)")
		return
	fi
	cp -f "$src" "$dest"
	echo "  windows: $what <- $src ($(du -h "$src" | cut -f1))"
}
copy_mfx "$WIN_EDITTIME" "$STAGE/windows/edittime/INI++.mfx" "edittime MFX"
copy_mfx "$WIN_RUNTIME" "$STAGE/windows/runtime/INI++.mfx" "runtime MFX"
cp -f packaging/install-windows.bat "$STAGE/windows/install.bat"

# -------------------------------------------------------------------------------------------
# Android natives + the Fusion Android extension package
# -------------------------------------------------------------------------------------------
find_so() { # <abi> -> path of the .so for that ABI, or empty
	local abi="$1"
	local candidate
	for candidate in "$ANDROID_DIR/assets/mmf/$abi/CRunINI++.so" \
	                 "$ANDROID_DIR/assets/mmf/$abi/CRunIniPlusPlus.so" \
	                 "$ANDROID_DIR/$abi/CRunINI++.so" \
	                 "$ANDROID_DIR/$abi/CRunIniPlusPlus.so"; do
		if [ -f "$candidate" ]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done
	return 1
}

abis=()
if [ -n "$ANDROID_DIR" ] && [ -d "$ANDROID_DIR" ]; then
	while IFS= read -r so; do
		abi="$(basename "$(dirname "$so")")"
		case " ${abis[*]-} " in
			*" $abi "*) continue ;;
		esac
		abis+=("$abi")
	done < <(find "$ANDROID_DIR" -name 'CRunINI++.so' -o -name 'CRunIniPlusPlus.so' 2>/dev/null | sort)
fi

if [ "${#abis[@]}" -eq 0 ]; then
	MISSING+=("Android natives (no CRunINI++.so found under ${ANDROID_DIR:-<no --android-dir>})")
else
	for abi in "${abis[@]}"; do
		src="$(find_so "$abi")"
		mkdir -p "$STAGE/android/assets/mmf/$abi"
		cp -f "$src" "$STAGE/android/assets/mmf/$abi/CRunINI++.so"
		echo "  android: $abi <- $src ($(du -h "$src" | cut -f1))"
	done
fi

# The Fusion package: use the one from the build if it is there, otherwise build it from the natives.
if [ -n "$ANDROID_DIR" ] && [ -f "$ANDROID_DIR/INI++.zip" ]; then
	cp -f "$ANDROID_DIR/INI++.zip" "$STAGE/android/INI++.zip"
	echo "  android: INI++.zip <- $ANDROID_DIR/INI++.zip"
elif [ "${#abis[@]}" -gt 0 ] && command -v zip >/dev/null 2>&1; then
	# INI++.zip must have assets/mmf/<abi>/ at its root (Data/Runtime/Android/<name>.zip)
	(cd "$STAGE/android" && zip -r -9 -q "INI++.zip" assets)
	echo "  android: INI++.zip built from ${#abis[@]} ABI(s)"
fi
[ -f "$STAGE/android/INI++.zip" ] || MISSING+=("android/INI++.zip (no natives and no prebuilt package)")

# -------------------------------------------------------------------------------------------
# Documentation, README, optional source
# -------------------------------------------------------------------------------------------
cp -f docs/INSTALL.md docs/COMPATIBILITY.md docs/BUILD.md "$STAGE/docs/"
sed -e "s/@VERSION@/$VERSION/" -e "s/@DATE@/$(date -u +%Y-%m-%d)/" \
	packaging/README.txt > "$STAGE/README.txt"
if [ "$WITH_SOURCE" = 1 ]; then
	mkdir -p "$STAGE/source"
	if git -C "$REPO_ROOT" rev-parse --verify HEAD >/dev/null 2>&1; then
		git -C "$REPO_ROOT" archive --format=tar.gz \
			--prefix="IniPlusPlus-source/" \
			-o "$STAGE/source/IniPlusPlus-$VERSION-source.tar.gz" HEAD
		echo "  source: IniPlusPlus-$VERSION-source.tar.gz ($(git rev-parse --short HEAD))"
	fi
fi

if [ "${#MISSING[@]}" -ne 0 ]; then
	{
		echo "This archive is INCOMPLETE - it was packaged with --allow-missing."
		echo
		echo "Missing:"
		for entry in "${MISSING[@]}"; do
			echo "  * $entry"
		done
		echo
		echo "Build them with the instructions in docs/BUILD.md, or run the CI workflow"
		echo "(.github/workflows/main.yml -> job 'package'), which produces a complete archive."
	} > "$STAGE/MISSING-BINARIES.txt"
fi

# -------------------------------------------------------------------------------------------
# Zip it up
# -------------------------------------------------------------------------------------------
rm -f "$ZIP"
if command -v zip >/dev/null 2>&1; then
	(cd "$OUT_DIR" && zip -r -9 -q "$NAME.zip" "$NAME")
	ZIP_BYTES="$(du -h "$ZIP" | cut -f1)"
else
	tar -C "$OUT_DIR" -czf "$ZIP.tgz" "$NAME"
	rm -rf "$ZIP"
	ZIP="$ZIP.tgz"
	ZIP_BYTES="$(du -h "$ZIP" | cut -f1)"
fi

echo
echo "Contents:"
if command -v unzip >/dev/null 2>&1; then
	unzip -l "$(dirname "$ZIP")/$(basename "$ZIP")" 2>/dev/null \
		| awk 'NR>3 && $4 != "" { printf "  %s\n", $4 }' | sed '$d' | head -40 \
		| sed "s|^  $NAME/|  |"
fi
echo
echo "Packaged: $ZIP ($ZIP_BYTES)"

if [ "${#MISSING[@]}" -ne 0 ]; then
	echo
	echo "error: incomplete package - missing:" >&2
	for entry in "${MISSING[@]}"; do
		echo "  * $entry" >&2
	done
	if [ "$ALLOW_MISSING" = 1 ]; then
		echo "(--allow-missing: wrote $NAME/MISSING-BINARIES.txt and continuing)" >&2
		exit 0
	fi
	echo "Pass --allow-missing only for layout/smoke tests. See docs/BUILD.md." >&2
	rm -f "$ZIP"
	exit 1
fi

echo "Install: run windows/install.bat inside the archive (Windows) and copy android/INI++.zip to"
echo "         <Fusion>/Data/Runtime/Android/ (Android). See the README.txt in the archive."
