#!/usr/bin/env bash
# Integrate the pure-Java AssArray subset into an extracted Fusion Android exporter project.
# This source/API guard does not replace or overwrite the vendor CExtLoad.java file.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXPORTER=""
CHECK_ONLY=0
ASSEMBLE=0

while [ $# -gt 0 ]; do
    case "$1" in
        --exporter) EXPORTER="${2:?--exporter needs an extracted exporter project directory}"; shift 2 ;;
        --exporter=*) EXPORTER="${1#*=}"; shift ;;
        --check-only) CHECK_ONLY=1; shift ;;
        --assemble-debug) ASSEMBLE=1; shift ;;
        -h|--help)
            cat <<'HELP'
Usage: tools/integrate_assarray_android_exporter.sh --exporter DIR [options]
  --check-only       validate source/API/registration markers without changing files
  --assemble-debug   integrate, then run the exporter's :app:assembleDebug task

Checks Fusion 293.0/295.10 Java API source and registers CRunAssArray in CExtLoad.java.
This is not a Fusion MFX build, a Sonic MFA export, or a device test.
HELP
            exit 0
            ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

if [ -z "$EXPORTER" ]; then
    echo "error: pass --exporter /path/to/an/extracted-Fusion-Android-Gradle-project" >&2
    exit 2
fi
if [ ! -d "$EXPORTER" ]; then
    echo "error: exporter project directory does not exist: $EXPORTER" >&2
    exit 2
fi
if [ "$CHECK_ONLY" = 1 ] && [ "$ASSEMBLE" = 1 ]; then
    echo "error: --check-only and --assemble-debug cannot be combined" >&2
    exit 2
fi
EXPORTER="$(cd "$EXPORTER" && pwd)"

find_unique() {
    local name="$1"
    local result
    result="$(find "$EXPORTER" -type f -path "*/app/src/main/java/*/$name" -print 2>/dev/null | head -2)"
    if [ "$(printf '%s\n' "$result" | sed '/^$/d' | wc -l)" -ne 1 ]; then
        echo "error: expected exactly one app/src/main/java/**/$name under $EXPORTER" >&2
        return 1
    fi
    printf '%s\n' "$result"
}
MMFRUNTIME="$(find_unique MMFRuntime.java)"
EXTLOAD="$(find_unique CExtLoad.java)"

python3 - "$MMFRUNTIME" "$EXTLOAD" "$EXPORTER" "$ROOT" "$CHECK_ONLY" "$ASSEMBLE" <<'PY'
from __future__ import annotations

import pathlib
import re
import shutil
import subprocess
import sys

mmf_path, extload_path, exporter_dir, repo_dir = map(pathlib.Path, sys.argv[1:5])
check_only = sys.argv[5] == "1"
assemble = sys.argv[6] == "1"

mmf_text = mmf_path.read_text(encoding="utf-8", errors="replace")
version_match = re.search(r'\bversion\s*=\s*"Fusion\s+([^" ]+)"', mmf_text)
version = version_match.group(1) if version_match else "unknown"
if version not in {"293.0", "295.10"}:
    raise SystemExit(
        f"error: exporter runtime source reports Fusion {version}; only inspected 293.0 and "
        "target 295.10 source labels are accepted. No files were changed."
    )

# Validate the Java API surface used by this adapter. These guards are not a javac compile.
java_root = mmf_path.parent.parent
checks = {
    "Extensions/CRunExtension.java": [
        "public abstract class CRunExtension", "getNumberOfConditions()",
        "createRunObject(CBinaryFile", "handleRunObject()", "destroyRunObject(boolean",
        "condition(int", "action(int", "expression(int",
    ],
    "Actions/CActExtension.java": ["getParamExpString", "getParamExpDouble"],
    "Conditions/CCndExtension.java": [],
    "Objects/CExtension.java": ["getControlsContext()", "getExpParam()"],
    "RunLoop/CCreateObjectInfo.java": [],
    "Services/CBinaryFile.java": [],
    "Expressions/CValue.java": ["CValue(int", "CValue(double", "getString()"],
}
for relative, needles in checks.items():
    path = java_root / relative
    if not path.is_file():
        raise SystemExit(f"error: required Fusion Java API file is missing: {path}")
    contents = path.read_text(encoding="utf-8", errors="replace")
    missing = [needle for needle in needles if needle not in contents]
    if missing:
        raise SystemExit(f"error: Fusion {version} Java API mismatch in {path}: missing {missing}")

extload = extload_path.read_text(encoding="utf-8", errors="replace")
registration = "object=new CRunAssArray();"
registration_present = registration in extload
android_branch = re.compile(
    r'else\s+if\s*\(\s*name\.compareToIgnoreCase\s*\(\s*"Android"\s*\)\s*==\s*0\s*\)'
)
android_matches = list(android_branch.finditer(extload))
if not registration_present and len(android_matches) != 1:
    raise SystemExit(f"error: expected one Android branch in {extload_path}; no files were changed")

print(f"Verified Fusion {version} Java API source")
print("Source guards passed (not a javac/Gradle compile).")
if check_only:
    print("check-only: no files changed")
    raise SystemExit(0)

# Never overwrite non-identical developer/vendor files.
def copy_without_overwrite(source: pathlib.Path, destination: pathlib.Path) -> None:
    if destination.exists():
        if destination.is_file() and source.is_file() and destination.read_bytes() == source.read_bytes():
            return
        raise SystemExit(f"error: refusing to overwrite existing exporter file: {destination}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)

source_dir = repo_dir / "android" / "java" / "Extensions"
target_dir = java_root / "Extensions"
for filename in ("CRunAssArray.java", "AssArrayIniStore.java"):
    copy_without_overwrite(source_dir / filename, target_dir / filename)

if not registration_present:
    insertion = (
        '\t\telse if (name.compareToIgnoreCase("ASSARRAY")==0)\n'
        '\t\t{\n'
        '\t\t\tobject=new CRunAssArray();\n'
        '\t\t}\n'
        '\t\telse if (name.compareToIgnoreCase("Android")==0)'
    )
    updated, count = android_branch.subn(insertion, extload, count=1)
    if count != 1:
        raise SystemExit("error: CExtLoad patch could not be applied; Java sources were copied, loader left unchanged")
    extload_path.write_text(updated, encoding="utf-8", newline="")
    print(f"registered CRunAssArray in {extload_path}")
else:
    print(f"CExtLoad registration already exists in {extload_path}")

print(f"Integrated AssArray Java sources into: {exporter_dir}")
print("This integration does not build Windows MFX files or test an exported MFA/device.")
if assemble:
    wrapper = exporter_dir / "gradlew"
    if not wrapper.is_file():
        raise SystemExit("error: --assemble-debug requested, but exporter/gradlew is missing")
    subprocess.run(["bash", str(wrapper), "--no-daemon", ":app:assembleDebug"], cwd=exporter_dir, check=True)
    apk_dir = exporter_dir / "app" / "build" / "outputs" / "apk"
    apks = sorted(apk_dir.rglob("*.apk")) if apk_dir.exists() else []
    if not apks:
        raise SystemExit(f"error: Gradle completed but no APK was found under {apk_dir}")
    print("Gradle produced:")
    for apk in apks:
        print(" ", apk)
PY
