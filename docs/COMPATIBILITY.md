# Ini++ on Android: architecture, compatibility and limitations

This document describes exactly what the Android build offers, what it does differently from the Windows build, and what has and has not been verified. The Windows extension is unchanged.

---

## 1. Approach

Ini++ is not rewritten and not re-implemented per platform: **the same `Runtime.cpp`, `ACEs.cpp` and `RunData.hpp` are compiled for Android**, with three additions:

1. **`android/fusion/` - a compile-time shim for the Windows build environment.** The shared sources include `<Windows.h>`, `<Shlobj.h>`, `lSDK.hpp` and the Fusion API headers, and call `mv*`/`CNC_*`/`callRunTimeFunction`. On Android these resolve to `android/fusion/FusionAPI.hpp` (types, constants, struct layouts taken from the Windows SDK headers so the shared code compiles unchanged), `ParamFrame.hpp` (the ACE parameter frame), and small stand-ins for the rest. Platform behaviour is behind `#ifdef FUSION_ANDROID_RUNTIME`; there is no `#ifdef` in any ACE body.
2. **`android/runtime/AndroidRuntime.cpp` - the Android platform layer:** the application's writable data directory, UTF-8 file I/O with the same BOM/encoding handling as Windows, logging, and `SerializedEditData::deserialize()` with the byte layout the Windows editor writes (v1 ANSI and v2 UTF-16), including the same "unknown version → defaults" behaviour (a log line instead of a message box).
3. **`android/jni/IniPlusPlusExtension.cc` + `android/runtime/AceDispatch.inc`** - the extension side of the official Android native extension ABI described by the Clickteam Android SDK (`RuntimeFunctions`, `extInit`/`createRunObject`/`getNumberOfConditions`/`destroyRunObject`/`handleRunObject`/`action`/`condition`/`expression`). The dispatch table is **generated from `Menus.cpp`** (`tools/gen_android_aces.py`), so parameter lists, types and their order are the same data the editor and the Windows runtime use, and the generator fails the build if `Menus.cpp`'s ACE lists and `ACEs.cpp`'s tables ever disagree.

Consequences: the ACE surface, the parameter order, the editing experience, the object's identifier (`0x12FD53A0`), its OEFLAGS, its icon and its editor resources are identical to Windows; a project only needs to be built for Android, not edited.

### 1.1 Parameter passing

| | Windows | Android |
| --- | --- | --- |
| Actions/Conditions (first two parameters) | `param0`/`param1` passed to the function | Condition parameters are read up front and handed over the same way; action parameters are read through the runtime's typed accessors (see below) and the same slots are used where an ACE reads `param0`/`param1` directly |
| Parameters read inside the ACE | `CNC_GetIntParameter` / `CNC_GetStringParameter` / `CNC_GetFloatParameter` cursor | the same `CNC_*` calls, forwarded to the Android runtime's `act_/cnd_/exp_getParam{Expression,ExpString,ExpFloat}` accessors, which advance exactly like the Windows cursor |
| Expressions | `CNC_GetFirst/NextExpressionParameter` with `TYPE_INT`/`TYPE_STRING`/`TYPE_FLOAT` | the same calls, forwarded to the runtime's accessors (the Android API evaluates and consumes one parameter per call, like the Windows runtime) |
| Expression return | `HOF_STRING` → returned string pointer; `HOF_FLOAT` → float bit pattern in an int32; otherwise an integer | identical |

This is why the "current group" variants of ACEs that read different parameters depending on which variant is called (e.g. `Set value` vs `Set value (current group)`, `Get item value` vs `Get item value (current group)`) behave correctly rather than reading the wrong parameter.

---

## 2. Verified, and how

| What | How it is verified | Status |
| --- | --- | --- |
| The shared sources compile for Android | `g++ -std=c++20 -DFUSION_ANDROID_RUNTIME ...` over `Runtime.cpp`/`ACEs.cpp` (also part of `tools/run_host_tests.sh`) | verified locally (GCC 12) |
| The ACE tables match `Menus.cpp`/`ACEs.cpp` (all 145 ACEs, same ids, same parameter order) | `python3 tools/gen_android_aces.py --check` (CI job `ace-tables`) | verified |
| Behaviour through the Android entry points: object creation from Windows-format edit data, groups/items, numbers and strings, `Set value (current group)`, `New`, `Load`, `Save as`, autosave via `handleRunObject`, expression returns, conditions | `tools/run_host_tests.sh` - 32 checks driving `CRunINI++_createRunObject/action/condition/expression/handleRunObject` through a mock of the Android runtime, including files written as UTF-16 and as Windows-1252 (CI job `host-tests`) | verified locally (GCC 12) |
| The extension is a loadable shared object exporting the exact entry points the runtime looks up (`CRunINI++_*` and `CRunIniPlusPlus_*`) | `tools/smoke_build_so.sh`: builds the `.so`, checks `nm -D`, then `dlopen`s it and resolves all 16 entry points with `dlsym` | verified locally |
| The Windows MFX still builds and exports its entry points | CI job `windows-extension` (MSVC v143, `Runtime`/`Edittime` Win32) | requires CI (needs MSVC) |
| The Android `.so` builds with the NDK against the official `<RuntimeNative.h>` | CI job `android-extension` (`ci/workflows/build.yml`) | requires CI + the Clickteam Android SDK |
| Behaviour on a real device / with the real runtime | Build an APK from an MFA that uses `INI++.mfx` with the exporter and run it | **not yet performed** - see section 4 |
| Windows behaviour is unaffected by the Android work | All changes to the shared sources are either Android-only (`#ifdef FUSION_ANDROID_RUNTIME`) or platform-neutral (`std::to_wstring` → `lSDK::string_t_from_numeric`, which is `std::to_wstring` on Windows); the Windows files, project file, resources and ACE tables are otherwise untouched | reviewed; Windows build runs in CI |

---

## 3. Differences and limitations

### 3.1 ACEs that are not implemented (on both platforms)

69 of the 145 ACEs have an implementation; the other 76 are stubs in `ACEs.cpp` (`NOT_YET_IMPLEMENTED`) and have never worked in this fork - on Windows they show an "unimplemented" message box, on Android they log

```
Ini++ (Android): ACE not implemented: <function name>
```

once per ACE and do nothing. They are still registered, still have their menus and their parameters, so an MFA that uses them keeps loading. The list (`tools/gen_android_aces.py --report`):

**Actions (54):** `Set string (MD5) (current group)`, `Save globals`, `Load globals`, `Rename group (current group)`, `Rename item (current group)`, `Move item to group (current group)`, `Set string (MD5)`, `Save object properties`, `Load object properties`, `Rename group`, `Rename item`, `Move item`, `Move item to group`, `Copy item`, `Delete item (everywhere)`, `Perform search`, `Perform multiple search`, `Clear results`, `Create sub-INI`, `Create sub-INI from object`, `Merge file`, `Merge group file`, `Merge`, `Merge group from object`, `Backup to`, `Set compression`, `Set read only`, `Set case sensitive`, `Set escape characters`, `Never quote strings`, `Set repeat modes`, `Set newline character`, `Set default directory`, `Compress file`, `Decompress file`, `Open dialog`, `Add repeated item`, `Close dialog`, `Refresh dialog`, `Export CSV`, `Import CSV`, `To chart`, `Find sub-groups`, `Enable sub-groups`, `SSS`, `Set item array`, `Load from array`, `Save to array`, `From chart`, `Save chart settings`, `Load chart settings`, `Clear undo stack`, `Add new undo block`, `Set manual mode`.

**Conditions (3):** `Compare MD5`, `Compare MD5 (current group)`, `Wildcard match`.

**Expressions (19):** `Search result count`, `Search result group`, `Search result item name`, `Search result item value`, `Search result item string`, `Search result path`, `Hash string`, `Encrypt string`, `Escape string`, `Unescape string`, `Inner product`, `Inner product (string)`, `Nth sorted name`, `Nth sorted value`, `CSV`, `Nth item (everywhere)`, `Unique item names`, `Item array`, `Current group (string)`.

Everything else works: `Set/Get value`, `Set/Get string`, the current-group variants, groups (exists/has item/item count/group count/total items/nth group/nth item/position), `New`, `Load file`, `Save`, `Save as`, `Close`, `Load from string`, `Auto save`, `Set read only`, `Undo`/`Redo` conditions (`Has undo`, `Has redo`), `Stringify`, `Empty`, `Item exists`, `Group exists`, `File name`, `Get part in string`, `Escape`/`Unescape`, `Fusion version` (`Get fusion version`), and the `On ...` event conditions, which return true when the runtime evaluates them - the shared implementation never triggered them on Windows either (nothing calls the runtime's generate-event function), so Android matches Windows here. If those events should be made to work, the Android side has `RuntimeFunctions::generateEvent` available.

### 3.2 Object / alterable-value ACEs

`Save object properties` / `Load object properties` (and the object based `Save`/`Load` actions) need to read and write **other objects' alterable values, strings, position, movement and flags**. The Android native extension API provides no access to other objects, so on Android (a) these ACEs are among the unimplemented ones above and (b) if they were reached, the object reference parameter would be the all-zero block. Nothing is ever dereferenced; the ACEs simply do nothing. This is the closest compatible behaviour available. The serialized object format (`val###`/`str###`/`posx`/`posy`/`dir`/`spd`/`ani`/`frame`/`flags` keys) is unchanged, so INI data written by those ACEs on Windows still *reads* fine on Android through the normal get-value/string expressions.

### 3.3 Custom parameters

Ini++ has ACEs with its own custom parameters (`Ini++ parameter: ...`). The Android extension API delivers no payload for them; such a parameter arrives as all-zero data (documented defaults) instead of the value chosen in the MFA. ACEs that use custom parameters for behaviour therefore behave like the "zero/default" case on Android. Check your events for "Ini++ parameter" entries if an ACE behaves unexpectedly.

### 3.4 Numeric precision

Fusion expressions are 64-bit doubles; the Android extension API hands extension code **32-bit floats** (`act/cnd/exp_getParamExpFloat`) and takes them back the same way (`exp_setReturnFloat`). Core arithmetic and comparisons of ordinary INI values are unaffected, but:

* a value written from an expression keeps ~7 significant digits instead of ~15 (e.g. `Set value` from an expression with a very precise decimal number),
* integers beyond 2^24 (16,777,216) can lose precision when they pass through an *expression parameter*. Values written through integer parameters (the usual case) keep the full 32-bit range.

Strings are unaffected: reads and writes are UTF-8 end to end.

### 3.5 Files and paths

* Relative names resolve against the application's private data directory (the same directory Fusion's own file object uses); absolute paths are used as given and follow Android's storage rules for the app.
* No `\\?\` long-path handling (a Windows-only workaround) - Android paths have no such prefix and the code path is skipped.
* The extension creates missing parent folders on save when "Can create folders" is enabled, as on Windows.
* INI files are byte-compatible with Windows in both directions: UTF-8 (with or without BOM) is Ini++'s format; UTF-16 LE/BE files written on Windows are converted on load; a file that is not valid UTF-8 (i.e. an ANSI/Windows-1252 file written by a Windows tool) is decoded as Windows-1252, which is what the Windows runtime does through the application's code page; escaped characters (`\\`, `\q`, `\=`, tabs, newlines, leading/trailing spaces) use the same escaping rules.
* Encrypted files (the `Encrypt` property) use the same Thayer cipher and stay readable on both platforms.

### 3.6 Edit data (object properties)

The blob the Windows editor writes is read on Android with the identical layout: v2 (Unicode, NUL-terminated UTF-16 strings) and v1 (the original ANSI version, fixed-size arrays, code page 1252). An unknown/garbled version logs and falls back to the object's default properties instead of showing a message box. Because the Android loader's pointer convention is not publicly documented, the extension accepts both a blob that starts with the extension header (`extHeader`) and a payload-only pointer by sanity-checking the header; both forms are covered by the host tests for the first form, and the second is a defensive fallback.

### 3.7 Lifecycle details

* `handleRunObject` (called every tick by the runtime) performs the same autosave decision as Windows.
* On destruction the object is destroyed with `fast = false`, so a pending autosave is written, like the Windows runtime does.
* `DisplayRunObject`, `PauseRunObject` etc. are not needed: the object is invisible on both platforms (OEFLAGS do not include drawing).

---

## 4. What is *not* verified yet

1. **A run on a real Android device / with the real Clickteam runtime.** The CI and host tests exercise the extension through the same entry points and parameter accessors, but the actual `RuntimeFunctions` implementation is proprietary and only exists inside the exporter's runtime. Two things can only be checked there: that the runtime resolves the `CRunINI++_*` symbols of the shipped `.so` (the smoke build proves the symbols exist and are resolvable by a dynamic loader), and which edit-data pointer convention this exporter version uses (both are handled).
2. **The APK packaging path.** The `INI++.zip` layout follows the Android SDK's documented extension package layout (`assets/mmf/<abi>/CRun<Name>.so`, `Data/Runtime/Android/<Name>.zip`), which is what the public SDK and its `tools/install-native` script use; a current exporter build should be checked by unzipping the APK as described in `docs/INSTALL.md` §2.3.
3. **Float precision differences** in the MFA-specific paths (§3.4) - by construction, not by test.
4. **The Windows MFX** builds in CI, but checking that Fusion can actually load it requires Windows with Fusion installed.

If any of these fail on a real setup, the failure modes are explicit (missing export → `dlopen`/`dlsym` error in logcat; wrong edit data → the "does not start with an Ini++ object header" log line; missing ACE → the "unknown <kind> id" log line), so the cause is identifiable from logcat.
