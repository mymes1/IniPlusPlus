#!/usr/bin/env python3
"""Static guard for the separate ASSARRAYSC2 editor/runtime and Android extension identities."""

from __future__ import annotations

import pathlib
import re
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parent.parent
MFX = ROOT / "extensions" / "AssArraySC2"
MENUS = (MFX / "Menus.cpp").read_text(encoding="utf-8")
GENERAL = (MFX / "General.cpp").read_text(encoding="utf-8")
ANDROID = (ROOT / "android/java/Extensions/CRunAssArraySC2.java").read_text(encoding="utf-8")
RC = (MFX / "Extension.rc").read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"error: {message}")


# The editor menu is intentionally the five ACEs from the SC2 target: four actions and one expression.
action_menu = MENUS.split("GetActionMenu", 1)[1].split("#endif", 1)[0]
expression_menu = MENUS.split("GetExpressionMenu", 1)[1].split("#endif", 1)[0]
action_ids = {int(value) for value in re.findall(r"Menu::Item\{(\d+), TSL\(", action_menu)}
expression_ids = {int(value) for value in re.findall(r"Menu::Item\{(\d+), TSL\(", expression_menu)}
require(action_ids == {0, 3, 12, 14}, f"unexpected action menu IDs: {sorted(action_ids)}")
require(expression_ids == {0}, f"unexpected expression menu IDs: {sorted(expression_ids)}")

signatures = {
    "lSDK::constant_ace_info<0, 0, PARAM_EXPSTRING, PARAM_EXPRESSION>()",
    "lSDK::constant_ace_info<3, 0>()",
    "lSDK::constant_ace_info<12, 0, PARAM_EXPSTRING>()",
    "lSDK::constant_ace_info<14, 0, PARAM_EXPSTRING>()",
    "lSDK::constant_ace_info<0, 0, EXPPARAM_STRING>()",
}
for signature in signatures:
    require(signature in MENUS, f"missing compatibility signature: {signature}")
require("numOfActions = static_cast<short>(std::size(actions))" in GENERAL,
        "runtime action count must be derived from the reserved ACE-ID table")
require("numOfConditions = static_cast<short>(std::size(conditions))" in GENERAL,
        "runtime condition count must match its legacy table")
require("numOfExpressions = static_cast<short>(std::size(expressions))" in GENERAL,
        "runtime expression count must match its legacy table")

def table_entries(array_name: str, entry_pattern: str) -> list[str]:
    match = re.search(
        rf"static fusion::[a-z_]+_func_pointer {array_name}\[\] = \{{(.*?)\n    \}};",
        GENERAL,
        re.DOTALL,
    )
    require(match is not None, f"missing runtime {array_name} dispatch table")
    return re.findall(entry_pattern, match.group(1))


action_entries = table_entries("actions", r"\baction_[a-z_]+\b")
condition_entries = table_entries("conditions", r"\bcondition_[a-z_]+\b")
expression_entries = table_entries("expressions", r"\bexpression_[a-z_]+\b")
require(len(action_entries) == 45,
        f"replacement action table must preserve all 45 original AssArray IDs, found {len(action_entries)}")
require(len(condition_entries) == 27,
        f"replacement condition table must preserve all 27 original AssArray IDs, found {len(condition_entries)}")
require(len(expression_entries) == 30,
        f"replacement expression table must preserve all 30 original AssArray IDs, found {len(expression_entries)}")
require(action_entries[0] == "action_set_value_at_key", "action ID 0 is not mapped to Set Value at Key")
require(action_entries[3] == "action_clear_array", "action ID 3 is not mapped to Clear the Array")
require(action_entries[12] == "action_load_ini_file_into_values", "action ID 12 is not mapped to Load INI")
require(action_entries[14] == "action_save_ini_file_from_values", "action ID 14 is not mapped to Save INI")
require(expression_entries[0] == "expression_get_value", "expression ID 0 is not mapped to GetValue")
require(all(name == "condition_unused" for name in condition_entries),
        "all unsupported condition slots must remain inert")
require(all(name == "action_unused" for index, name in enumerate(action_entries)
            if index not in {0, 3, 12, 14}),
        "all non-SC2 action slots must remain inert")
require(all(name == "expression_unused" for name in expression_entries[1:]),
        "all non-SC2 expression slots must remain inert")

# Windows editor object, Android exporter registration and package all use the separate identity.
require('KPX_NAME "ASSARRAYSC2"' in RC, "MFX KPX_NAME must not register as ASSARRAY")
require("AS2C" in GENERAL, "MFX runtime must use a distinct AS2C identifier")
require("class CRunAssArraySC2" in ANDROID, "Android runtime class is missing")
require("ACTION_SET_VALUE_AT_KEY = 0" in ANDROID, "Android action 0 mapping is missing")
require("ACTION_CLEAR_ARRAY = 3" in ANDROID, "Android action 3 mapping is missing")
require("ACTION_LOAD_INI_INTO_VALUES = 12" in ANDROID, "Android action 12 mapping is missing")
require("ACTION_SAVE_INI_FROM_VALUES = 14" in ANDROID, "Android action 14 mapping is missing")
require("EXPRESSION_GET_VALUE = 0" in ANDROID, "Android expression 0 mapping is missing")
require("object=new CRunAssArraySC2();" in (ROOT / "tools/integrate_assarray_sc2_android_exporter.sh").read_text(encoding="utf-8"),
        "Android exporter helper does not register the new class")
require((ROOT / "tools/package_assarray_sc2_android_extension.sh").is_file(),
        "new Android package helper is missing")
require(not (ROOT / "android/java/Extensions/CRunAssArray.java").exists(),
        "obsolete ASSARRAY-identity Android prototype must not remain")

project = ET.parse(MFX / "AssArraySC2.vcxproj").getroot()
ns = {"msb": "http://schemas.microsoft.com/developer/msbuild/2003"}
configurations = {
    node.findtext("msb:Configuration", namespaces=ns)
    for node in project.findall(".//msb:ProjectConfiguration", ns)
}
require({"Runtime", "Edittime"}.issubset(configurations), "MFX project must define Runtime and Edittime builds")
project_text = (MFX / "AssArraySC2.vcxproj").read_text(encoding="utf-8")
require("mmfs2u.lib" in project_text and "lSDK\\include" in project_text,
        "independent MFX project must use the checked-in Fusion SDK only")
require(project_text.count("<LanguageStandard>stdcpp20</LanguageStandard>") == 3,
        "all MFX configurations must use C++20 for lSDK FusionMenu's std::span")
require(project_text.count("<ConformanceMode>false</ConformanceMode>") == 3,
        "all MFX configurations must use MSVC permissive lookup for lSDK FusionParams")
require("AssArray.mfx" not in project_text and "AssArrayruntime.mfx" not in project_text,
        "new MFX project must not require original AssArray binaries")

print("ASSARRAYSC2 static ACE identity/signature checks passed (not an MSVC build or SC2 save test).")
