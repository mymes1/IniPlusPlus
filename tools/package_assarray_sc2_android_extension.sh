#!/usr/bin/env bash
# Package the Java runtime for the separate ASSARRAYSC2 editor/runtime extension object.
# The Android adapter is pure Java and needs no NDK library.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$ROOT/build/android-package"
INSTALL_TO=""
REPLACE_INSTALLED=0

while [ $# -gt 0 ]; do
    case "$1" in
        --out) OUT_DIR="${2:?--out needs a directory}"; shift 2 ;;
        --out=*) OUT_DIR="${1#*=}"; shift ;;
        --install-to) INSTALL_TO="${2:?--install-to needs the Fusion Data/Runtime/Android directory}"; shift 2 ;;
        --install-to=*) INSTALL_TO="${1#*=}"; shift ;;
        --replace-installed) REPLACE_INSTALLED=1; shift ;;
        -h|--help)
            cat <<'HELP'
Usage: tools/package_assarray_sc2_android_extension.sh [options]
  --out DIR               ZIP output directory (default: build/android-package)
  --install-to DIR        install ASSARRAYSC2.zip into Fusion's Data/Runtime/Android directory
  --replace-installed     allow replacing an existing ASSARRAYSC2.zip (back it up first)

This source package implements only the five numeric AssArray ACEs used by Sonic Classic 2.
It does not build Windows MFX files.
HELP
            exit 0
            ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

if ! command -v zip >/dev/null 2>&1 || ! command -v unzip >/dev/null 2>&1; then
    echo "error: zip and unzip are required" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
OUT_DIR="$(cd "$OUT_DIR" && pwd)"
STAGE="$OUT_DIR/assarraysc2-stage"
rm -rf "$STAGE"
mkdir -p "$STAGE/src/Extensions"
cp "$ROOT/android/java/Extensions/CRunAssArraySC2.java" "$STAGE/src/Extensions/"
cp "$ROOT/android/java/Extensions/AssArraySC2IniStore.java" "$STAGE/src/Extensions/"

cat > "$STAGE/README.txt" <<'EOF'
AssArray SC2-compatible Android runtime for Sonic Classic 2

Source package layout:
  src/Extensions/CRunAssArraySC2.java
  src/Extensions/AssArraySC2IniStore.java

ACE IDs matching the AssArray ACE IDs used by Sonic Classic 2:
  action 0: Set Value at Key
  action 3: Clear the Array
  action 12: Load INI File into Values
  action 14: Save INI File from Values
  expression 0: GetValue

The adapter reports the original 12-condition table size, but conditions are not implemented; the
inspected Sonic Classic 2 export does not use AssArray conditions. Other AssArray features are not
included in this subset.

The numeric INI implementation follows the flat key=value format in ringdata.sav from Sonic
Classic 1: case-insensitive keys normalized to lowercase, CRLF output, and no section header. That
file is only a format fixture and does not establish Sonic Classic 2 save compatibility. Paths are
mapped into the Android app's private files directory. The target MFA must be exported and tested
against an actual Sonic Classic 2 save on a device before target-game compatibility can be claimed.

Install this ZIP as:
  <Fusion 2.5>/Data/Runtime/Android/ASSARRAYSC2.zip

The target exporter's Extensions/CExtLoad.java must register CRunAssArraySC2 for the ASSARRAYSC2
extension name. Use tools/integrate_assarray_sc2_android_exporter.sh on an extracted exporter project
to perform the source copy and registration without replacing the vendor loader.

This source package has not yet been compiled by Fusion 295.10, exported with the target MFA, or
tested on a device. It does not contain or build Windows .mfx files.
EOF

ZIP="$OUT_DIR/ASSARRAYSC2.zip"
rm -f "$ZIP"
(cd "$STAGE" && zip -r -9 -q "$ZIP" src README.txt)
ENTRIES="$(unzip -Z1 "$ZIP")"
for required in src/Extensions/CRunAssArraySC2.java src/Extensions/AssArraySC2IniStore.java README.txt; do
    if ! grep -Fxq "$required" <<<"$ENTRIES"; then
        echo "error: ASSARRAYSC2.zip is missing required entry: $required" >&2
        exit 1
    fi
done
if grep -Eiq '\.(class|jar|aar)$|(^|/)AndroidSDK_Gradle\.zip$' <<<"$ENTRIES"; then
    echo "error: refusing to package compiled Java or proprietary exporter artifacts" >&2
    exit 1
fi

echo "created $ZIP"
unzip -l "$ZIP"

if [ -n "$INSTALL_TO" ]; then
    mkdir -p "$INSTALL_TO"
    DEST="$INSTALL_TO/ASSARRAYSC2.zip"
    if [ -e "$DEST" ] && [ "$REPLACE_INSTALLED" = 0 ]; then
        echo "error: refusing to overwrite $DEST; pass --replace-installed after making a backup" >&2
        exit 1
    fi
    cp -f "$ZIP" "$DEST"
    echo "installed Fusion Android extension ZIP: $DEST"
fi
