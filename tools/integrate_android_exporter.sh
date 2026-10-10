#!/usr/bin/env bash
# Copy the Java adapter and NDK library into an extracted Fusion 2.5 Android Gradle project,
# then register CRunIniPlusPlus in that project's CExtLoad.java. The script is deliberately pinned
# to the requested Fusion 2.5 build 295.10 and refuses other source trees.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXPORTER=""
ANDROID_DIR="$ROOT/build/android"
CHECK_ONLY=0
ASSEMBLE=0

while [ $# -gt 0 ]; do
	case "$1" in
		--exporter) EXPORTER="${2:?--exporter needs an extracted exporter project directory}"; shift 2 ;;
		--exporter=*) EXPORTER="${1#*=}"; shift ;;
		--android-dir) ANDROID_DIR="${2:?--android-dir needs a build_android.sh output directory}"; shift 2 ;;
		--android-dir=*) ANDROID_DIR="${1#*=}"; shift ;;
		--check-only) CHECK_ONLY=1; shift ;;
		--assemble-debug) ASSEMBLE=1; shift ;;
		-h|--help)
			sed -n '2,8p' "$0"
			exit 0
			;;
		*) echo "unknown argument: $1" >&2; exit 2 ;;
	esac
done

if [ -z "$EXPORTER" ]; then
	echo "error: pass --exporter /path/to/the/extracted-Fusion-295.10-Gradle-project" >&2
	exit 2
fi
if [ ! -d "$EXPORTER" ]; then
	echo "error: exporter project directory does not exist: $EXPORTER" >&2
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

python3 - "$MMFRUNTIME" "$EXTLOAD" "$EXPORTER" "$ANDROID_DIR" "$ROOT" "$CHECK_ONLY" "$ASSEMBLE" <<'PY'
from __future__ import annotations
import pathlib
import re
import shutil
import subprocess
import sys

mmf_path, extload_path, exporter_dir, android_dir, repo_dir = map(pathlib.Path, sys.argv[1:6])
check_only = sys.argv[6] == "1"
assemble = sys.argv[7] == "1"

mmf = mmf_path.read_text(encoding="utf-8", errors="replace")
match = re.search(r'\bversion\s*=\s*"Fusion\s+([^" ]+)"', mmf)
version = match.group(1) if match else "unknown"
if version != "295.10":
    raise SystemExit(
        f"error: exporter runtime source reports Fusion {version}, not the required Fusion 295.10. "
        "No files were changed. The repository's AndroidSDK_Gradle.zip currently reports Fusion "
        "292.0; it is not a substitute for the selected 295.10 exporter. Obtain the exact authorized "
        "295.10 Gradle exporter project and retry."
    )

# Validate the subset of the Java API this adapter calls. This is a source guard, not a Java compile.
java_root = mmf_path.parent.parent
checks = {
    "Extensions/CRunExtension.java": ["public abstract class CRunExtension", "getNumberOfConditions()", "createRunObject(CBinaryFile"],
    "Actions/CActExtension.java": ["getParamExpString", "getParamExpDouble", "getParamExpression", "getParamPosition", "getParamFilename", "getParamFilename2"],
    "Conditions/CCndExtension.java": ["getParamExpString", "getParamExpDouble", "getParamExpression", "getParamPosition", "getParamFilename", "getParamFilename2"],
    "Services/CBinaryFile.java": ["byte data[]"],
    "Expressions/CValue.java": ["CValue(int", "CValue(double", "CValue(String"],
}
for relative, needles in checks.items():
    path = java_root / relative
    if not path.is_file():
        raise SystemExit(f"error: required Fusion Java API file is missing: {path}")
    text = path.read_text(encoding="utf-8", errors="replace")
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise SystemExit(f"error: Fusion 295.10 Java API mismatch in {path}: missing {missing}")

extload = extload_path.read_text(encoding="utf-8", errors="replace")
registration = "object=new CRunIniPlusPlus();"
marker = re.compile(r'else\s+if\s*\(\s*name\.compareToIgnoreCase\s*\(\s*"Android"\s*\)\s*==\s*0\s*\)')
if registration not in extload and len(list(marker.finditer(extload))) != 1:
    raise SystemExit(f"error: expected one Android branch in {extload_path}; no files were changed")

assets = android_dir / "assets" / "mmf"
abi_dirs = sorted(path for path in assets.iterdir() if path.is_dir()) if assets.is_dir() else []
if not abi_dirs:
    raise SystemExit(f"error: no built NDK assets at {assets}; run tools/build_android.sh first")
for abi_dir in abi_dirs:
    library = abi_dir / "libIniPlusPlusBridge.so"
    if not library.is_file():
        raise SystemExit(f"error: required JNI library is missing: {library}")

wrapper = exporter_dir / "gradle" / "wrapper" / "gradle-wrapper.properties"
root_gradle = exporter_dir / "build.gradle"
gradle_version = "unknown"
plugin_version = "unknown"
if wrapper.is_file():
    gradle_match = re.search(r"^distributionUrl=.*gradle-([^/]+?)-(?:all|bin)\.zip", wrapper.read_text(encoding="utf-8", errors="replace"), re.M)
    if gradle_match:
        gradle_version = gradle_match.group(1)
if root_gradle.is_file():
    plugin_match = re.search(r"com\.android\.tools\.build:gradle:([^'\"]+)", root_gradle.read_text(encoding="utf-8", errors="replace"))
    if plugin_match:
        plugin_version = plugin_match.group(1)
print(f"Verified target: Fusion {version}; wrapper Gradle: {gradle_version}; Android Gradle Plugin: {plugin_version}")
print("Java API source guards passed (not a javac/Gradle compile).")
print("Android ABIs:", ", ".join(path.name for path in abi_dirs))
if check_only:
    print("check-only: no files changed")
    raise SystemExit(0)

# Refuse to replace a developer's existing source/library unless it is byte-identical.
def copy_without_overwrite(source: pathlib.Path, destination: pathlib.Path) -> None:
    if destination.exists():
        if destination.is_file() and source.is_file() and destination.read_bytes() == source.read_bytes():
            return
        raise SystemExit(f"error: refusing to overwrite existing exporter file: {destination}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)

adapter = repo_dir / "android" / "java" / "Extensions"
java_dest = java_root / "Extensions"
for filename in ("CRunIniPlusPlus.java", "IniPlusPlusAceParams.java"):
    copy_without_overwrite(adapter / filename, java_dest / filename)

for abi_dir in abi_dirs:
    destination = exporter_dir / "app" / "src" / "main" / "assets" / "mmf" / abi_dir.name / "libIniPlusPlusBridge.so"
    copy_without_overwrite(abi_dir / "libIniPlusPlusBridge.so", destination)

if registration not in extload:
    branch = "\n".join([
        'else if (name.compareToIgnoreCase("INI++")==0 || name.compareToIgnoreCase("Ini++_Unicode")==0)',
        "\t\t{",
        "\t\t\tobject=new CRunIniPlusPlus();",
        "\t\t}",
        '\t\telse if (name.compareToIgnoreCase("Android")==0)',
    ])
    updated, count = marker.subn(branch, extload, count=1)
    if count != 1:
        raise SystemExit("error: CExtLoad patch could not be applied; Java files/libs were copied, loader left unchanged")
    with extload_path.open("w", encoding="utf-8", newline="") as output:
        output.write(updated)
    print(f"registered CRunIniPlusPlus in {extload_path}")
else:
    print(f"CExtLoad registration already exists in {extload_path}")

print(f"Integrated Ini++ Java/JNI sources into: {exporter_dir}")
print("This integration does not export an MFA or test an APK/device.")
if assemble:
    wrapper_script = exporter_dir / "gradlew"
    if not wrapper_script.is_file():
        raise SystemExit("error: --assemble-debug requested, but exporter/gradlew is missing")
    subprocess.run([str(wrapper_script), "--no-daemon", ":app:assembleDebug"], cwd=exporter_dir, check=True)
    apk_dir = exporter_dir / "app" / "build" / "outputs" / "apk"
    apks = sorted(apk_dir.rglob("*.apk")) if apk_dir.exists() else []
    if not apks:
        raise SystemExit(f"error: Gradle completed but no APK was found under {apk_dir}")
    print("Gradle produced:")
    for apk in apks:
        print(" ", apk)
PY
