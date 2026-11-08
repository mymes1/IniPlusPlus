#!/usr/bin/env bash
# Smoke-test the independent ASSARRAYSC2 Android source package and its non-colliding identity.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/assarraysc2-package.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

bash "$ROOT/tools/package_assarray_sc2_android_extension.sh" --out "$TMP/out" >/dev/null
ZIP="$TMP/out/ASSARRAYSC2.zip"
[ -f "$ZIP" ] || { echo "error: ASSARRAYSC2.zip was not created" >&2; exit 1; }
ENTRIES="$(unzip -Z1 "$ZIP")"
for required in \
    src/Extensions/CRunAssArraySC2.java \
    src/Extensions/AssArraySC2IniStore.java \
    README.txt; do
    if ! grep -Fxq "$required" <<<"$ENTRIES"; then
        echo "error: ASSARRAYSC2.zip is missing $required" >&2
        exit 1
    fi
done
if grep -Eiq '\.(class|jar|aar|mfx)$|AssArray\.mfx|AssArrayruntime\.mfx|(^|/)CRunAssArray\.java$' <<<"$ENTRIES"; then
    echo "error: ASSARRAYSC2.zip contains a compiled/original AssArray artifact or wrong runtime class" >&2
    exit 1
fi
README="$(unzip -p "$ZIP" README.txt)"
grep -Fq 'ASSARRAYSC2.zip' <<<"$README"
grep -Fq 'format fixture' <<<"$README"
grep -Fq 'Classic 1' <<<"$README"
echo "ASSARRAYSC2 Android package test passed."
