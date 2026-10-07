#!/usr/bin/env python3
"""Generate the Android ACE dispatch table for Ini++.

The Windows/MFX build dispatches ACEs through the tables at the bottom of ACEs.cpp: Fusion knows
the ACE id, and the SDK macros read the parameters in the order declared in Menus.cpp.

The Fusion 2.5 Android native extension API instead hands the extension only the ACE id and typed
sequential parameter accessors, so the extension must read each parameter itself, using the type
the ACE declares for it.  Rather than duplicating that knowledge in hand-written code, this script
reads

  * Menus.cpp: the authoritative ACE id -> parameter type declaration order
  * ACEs.cpp:  the authoritative ACE id -> C++ function mapping

and emits android/runtime/AceDispatch.inc: forward declarations of every ACE, the parameter type
table, and the three switch statements (action/condition/expression) that read the parameters and
invoke the shared ACE bodies from ACEs.cpp.

Usage:  python3 tools/gen_android_aces.py [--check]
"""

from __future__ import annotations

import argparse
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT = os.path.join(REPO, "android", "runtime", "AceDispatch.inc")
DECLS = os.path.join(REPO, "android", "runtime", "AceDeclarations.inc")


def read(path: str) -> str:
    with open(os.path.join(REPO, path), encoding="utf-8-sig", errors="replace") as f:
        return f.read()


def parse_ace_infos(src: str, table: str) -> list[tuple[int, list[str]]]:
    """Parse `constant_ace_info<id, menu, display, flags[, ConstantAceParam<type, name>...]>()`."""
    m = re.search(r"static constexpr std::array const " + table + r"\s*\{(.*?)\n\};", src, re.S)
    if not m:
        raise SystemExit(f"error: could not find {table}")
    out: list[tuple[int, list[str]]] = []
    body, start = m.group(1), 0
    while True:
        at = body.find("constant_ace_info<", start)
        if at < 0:
            break
        i, depth = at + len("constant_ace_info"), 0
        begin = i
        while i < len(body):
            if body[i] == "<":
                depth += 1
            elif body[i] == ">":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        args = body[begin + 1 : i]
        start = i + 1
        if args.lstrip().startswith("int"):  # forward declaration, not an entry
            continue
        cid = int(re.match(r"\s*(\d+)\s*,", args).group(1))
        out.append((cid, re.findall(r"ConstantAceParam<\s*([A-Z0-9_]+)\s*,", args)))
    for expected, (cid, _) in enumerate(out):
        if cid != expected:
            raise SystemExit(f"error: {table} is not in id order ({cid} where {expected} expected)")
    return out


def parse_functions(src: str, table: str) -> list[str]:
    m = re.search(r"std::vector<fusion::\w+_func_pointer> const RunData::" + table + r"\s*\{(.*?)\n\};", src, re.S)
    if not m:
        raise SystemExit(f"error: could not find RunData::{table}")
    return [l.split("*/", 1)[1].strip().rstrip(",") for l in m.group(1).splitlines() if "/*" in l]


# parameter type -> reader call; see lSDK/include/FusionAPI/Cncf.h and CustomParams.hpp
STRING_TYPES = {"PARAM_EXPSTRING", "PARAM_STRING", "PARAM_FILENAME", "PARAM_FILENAME2", "EXPPARAM_STRING"}
NUMBER_TYPES = {"PARAM_EXPRESSION", "PARAM_POSITION", "EXPPARAM_LONG"}


def reader_for(param: str) -> str:
    if param in STRING_TYPES:
        return "add_string"
    if param in NUMBER_TYPES:
        return "add_number"
    # PARAM_OBJECT and Ini++'s own custom parameters (PARAM_EXTBASE + n, see CustomParams.hpp)
    # cannot be delivered by the Android extension API; ACEs that use them receive the zeroed
    # block from ParamFrame::cnc_get_object() (documented defaults).
    return "add_object"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true", help="fail if the generated file is out of date")
    ap.add_argument(
        "--report",
        action="store_true",
        help="print which ACEs are implemented on Android and which are stubs (docs/COMPATIBILITY.md)",
    )
    args = ap.parse_args()

    menus, aces = read("Menus.cpp"), read("ACEs.cpp")

    tables = [
        ("action", "action_infos", "ACTIONS", "action"),
        ("condition", "condition_infos", "CONDITIONS", "condition"),
        ("expression", "expression_infos", "EXPRESSIONS", "expression"),
    ]

    if args.report:
        # Which of the shared ACE bodies actually do something on Android: ACEs.cpp's stubs are
        # marked with NOT_YET_IMPLEMENTED.  The dispatch table calls them all either way; the stubs
        # log and do nothing.
        for kind, infos_name, functions_name, _ in tables:
            infos = parse_ace_infos(menus, infos_name)
            functions = parse_functions(aces, functions_name)
            implemented, stubs = [], []
            for cid, _params in infos:
                name = functions[cid]
                match = re.search(
                    rf"auto FUSION_API {name}\s*\(.*?\n\{{(.*?)\n\}}",
                    aces,
                    re.S,
                )
                body = match.group(1) if match else ""
                (stubs if "NOT_YET_IMPLEMENTED" in body else implemented).append(f"{cid}:{name}")
            print(f"== {kind}s: {len(implemented)} implemented, {len(stubs)} stubs")
            for name in stubs:
                print(f"   stub    {name}")
    

    out: list[str] = []
    decls: list[str] = []
    w = out.append
    d = decls.append
    w("// AUTO-GENERATED by tools/gen_android_aces.py - DO NOT EDIT.")
    w("// Regenerate after changing an ACE declaration in Menus.cpp or an ACE in ACEs.cpp:")
    w("//     python3 tools/gen_android_aces.py")
    w("//")
    w("// For every ACE this file provides")
    w("//   * a forward declaration of the shared ACE body from ACEs.cpp,")
    w("//   * the parameter list (types and order) taken from Menus.cpp, and")
    w("//   * a dispatch case that reads those parameters in that order and calls the ACE body.")
    w("")
    parsed: dict[str, list[str]] = {}

    for kind, infos_name, functions_name, _ in tables:
        infos = parse_ace_infos(menus, infos_name)
        functions = parse_functions(aces, functions_name)
        if len(infos) != len(functions):
            raise SystemExit(
                f"error: {infos_name} declares {len(infos)} ACEs but {functions_name} implements {len(functions)}"
            )
        parsed[kind] = functions
        d(f"// ---- {kind} ACEs ({len(infos)}) ------------------------------------------------------------------")
        return_type = f"::fusion::{kind}_return_t"
        for cid, params in infos:
            name = functions[cid]
            ace_args = (
                f"::RunData* const, ::fusion::ac_param_t const, ::fusion::ac_param_t const"
                if kind != "expression"
                else f"::RunData* const, ::fusion::e_params_t const"
            )
            d(f"auto FUSION_API {name}({ace_args}) noexcept -> {return_type}; // {cid}")
        d("")

    w("// This file is included from inside namespace ipp_android (see IniPlusPlusExtension.cc).")
    for kind, infos_name, functions_name, invoke in tables:
        infos = parse_ace_infos(menus, infos_name)
        functions = parsed[kind]
        w(f"inline ::fusion::{kind}_return_t dispatch_{kind}(std::int32_t const num, ::RunData* const run_data,")
        w(f"\tParamFrame& frame, [[maybe_unused]] AceParamReader& reader) noexcept")
        w("\t{")
        w("\t\tframe.reset();")
        w("\t\tswitch(num)")
        w("\t\t{")
        for cid, params in infos:
            name = functions[cid]
            w(f"\t\t\tcase {cid}: // {name}")
            if kind != "expression":
                # Actions and conditions receive their first two parameters as the raw param0 and
                # param1 slots (several ACEs use them directly), and read the rest through the
                # parameter cursor; both need the declared parameters evaluated up front.
                # Expressions read their parameters lazily, in the order the ACE asks for them,
                # exactly like the Windows implementation (some expression variants skip a
                # parameter depending on which ACE number they were called as).
                for param in params:
                    w(f"\t\t\t\treader.{reader_for(param)}(frame.params);")
            w(f"\t\t\t\trun_data->rHo.hoEventNumber = frame.ace_number = {cid};")
            if kind == "expression":
                w(f"\t\t\t\treturn {name}(run_data, 0);")
            else:
                w(f"\t\t\t\treturn {name}(run_data, frame.slot(0), frame.slot(1));")
        w("\t\t\tdefault:")
        w(f'\t\t\t\tlog(\"Ini++: unknown {kind} id \" + std::to_string(num));')
        w(f"\t\t\t\treturn ::fusion::{kind}_return_t{{}};")
        w("\t\t}")
        w("\t}")
    w("")

    counts = {kind: len(parsed[kind]) for kind, *_ in tables}
    decls.insert(0, "// This file is included at global scope (see IniPlusPlusExtension.cc).")
    decls.insert(1, "namespace ipp_android")
    decls.insert(2, "{")
    decls.insert(3, f"\tinline constexpr int number_of_actions     = {counts['action']};")
    decls.insert(4, f"\tinline constexpr int number_of_conditions  = {counts['condition']};")
    decls.insert(5, f"\tinline constexpr int number_of_expressions = {counts['expression']};")
    decls.insert(6, "}")
    decls.insert(7, "")

    generated = "\n".join(out)
    generated_decls = "\n".join(decls)

    def write_if_changed(path: str, text: str) -> bool:
        try:
            if open(path, encoding="utf-8").read() == text:
                return False
        except FileNotFoundError:
            pass
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
        return True

    if args.check:
        for path in (OUTPUT, DECLS):
            try:
                if open(path, encoding="utf-8").read() != (generated if path == OUTPUT else generated_decls):
                    print(f"{os.path.relpath(path, REPO)} is out of date - run tools/gen_android_aces.py", file=sys.stderr)
                    return 1
            except FileNotFoundError:
                print(f"{os.path.relpath(path, REPO)} is missing - run tools/gen_android_aces.py", file=sys.stderr)
                return 1
        print("the generated Android ACE tables are up to date")
        return 0

    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    write_if_changed(OUTPUT, generated)
    write_if_changed(DECLS, generated_decls)
    print(f"wrote android/runtime/AceDispatch.inc and android/runtime/AceDeclarations.inc "
          f"({len(generated.splitlines())} + {len(generated_decls.splitlines())} lines)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
