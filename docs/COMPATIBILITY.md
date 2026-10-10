# Ini++ Android compatibility and verification status

## Status first

The Android path is an **unfinished source implementation**, not a completed Fusion extension release. The current direction is a Java `CRunExtension` adapter calling the shared C++ ACE bodies through this project's JNI library. Host C++ bridge tests pass. Java/Gradle compilation against the requested Fusion 2.5 build **295.10**, NDK compilation, Fusion `CExtLoad` registration, exporter ZIP merge, Sonic MFA APK export and a device test have not been completed.

The only Gradle runtime archive currently at the repository root reports **Fusion 292.0** (`MMFRuntime.java`), with Gradle 4.10.1 / AGP 3.3.1. It is not the requested 295.10 exporter. The integration script fails closed on that mismatch. Do not substitute that runtime, include the proprietary ZIP in runtime artifacts, or claim Android support complete.

## 1. Architecture

- `Runtime.cpp`, `ACEs.cpp`, `RunData.hpp`, `Settings.hpp` and the `Menus.cpp` ACE declarations are shared with the Windows MFX.
- `android/java/Extensions/CRunIniPlusPlus.java` implements the Java `CRunExtension` surface. It reads action/condition/expression parameters with the Java runtime's typed APIs and holds the object lifecycle.
- `android/jni/JniBridge.cpp` converts Java values to UTF-8/bridge input and returns `Expressions.CValue` results.
- `android/runtime/Bridge.cpp` owns the shared C++ run object and dispatches the existing ACE functions. `tools/gen_android_aces.py` derives the ordered Java/C++ parameter tables from `Menus.cpp` and the shared ACE tables.
- `android/runtime/AndroidRuntime.cpp` provides Android app-private file access, file I/O, logging and edit-data decoding. `safe_data_path()` checks paths before text I/O, encrypted direct I/O and parent-directory creation.
- The Android build uses JNI and Java registration, **not** Clickteam's historical native `RuntimeNative.h` ABI. The project's Android compatibility declarations are not a replacement Fusion runtime or C++ SDK.

`CExtLoad.java` in the exporter must register `CRunIniPlusPlus`; the extension is not loaded by a compile define alone. `tools/integrate_android_exporter.sh` copies the Java adapter and `.so` files and patches that registration only after confirming the exporter source identifies as Fusion 295.10. That successful path is not yet tested.

## 2. ACE, edit-data and Windows compatibility

The generated table currently contains **84 actions, 20 conditions and 41 expressions**, matching the checked-in `Menus.cpp` / `ACEs.cpp` declarations and order. The host test covers representative string/number/condition/current-group/position dispatch. Run `python3 tools/gen_android_aces.py --check` after ACE edits.

The Windows MFX project, Windows editor files, icon, object identifier and resource strings are kept in the tree; Android changes use separate Java/JNI/platform sources and `FUSION_ANDROID_RUNTIME` guards. The Windows MFX has **not** been built in this continuation, so Windows non-regression is not yet established by a build result.

The Android deserializer retains the shared v1 ANSI and v2 Unicode edit-data format. The bridge accepts either a full `extHeader`-prefixed blob or a payload-only byte array. The current host tests cover payload-only v1 ANSI data (Windows-1252) and v2 Unicode data; the target runtime's precise pointer convention still needs a compatible 295.10 exporter test. Do not infer successful MFA object-data loading from the host test alone.

## 3. Known Android behavior differences

### 3.1 Object references and custom parameters

The Java runtime APIs expose object/custom parameter accessors, but this adapter currently maps those parameter kinds to safe placeholders instead of marshalling Java objects and `PARAM_EXTBASE` payloads into the C++ bridge. The parameter indexes/ACE declarations stay present, but some behavior does not match Windows:

- The four `Save/Load object properties` ACEs (with and without group variants) are guarded as no-ops on Android. They do not save or mutate a placeholder's zero values.
- ACEs that use Ini++ custom parameter payloads receive zero/default placeholders.
- Other ACEs that depend on these values must be identified during project testing; do not assume every event in every existing MFA behaves identically on Android.

This is an active compatibility gap and could require event changes in projects that use these features. It must be closed or explicitly accepted after testing the Sonic MFA before claiming the user's normal-MFA/no-event-change acceptance criterion.

### 3.2 Existing unimplemented ACE bodies

Several ACE implementations in this fork are already marked `NOT_YET_IMPLEMENTED` in shared `ACEs.cpp` and are not functional on Windows either. Android preserves their IDs and menu/parameter definitions; it does not add missing semantics. Use `python3 tools/gen_android_aces.py --report` to inspect the declaration/implementation status.

### 3.3 Numbers and strings

Java expression APIs produce `CValue` objects and the JNI bridge preserves string values as UTF-8. The shared Fusion ACE code returns its float expression values through a 32-bit float representation to match the existing Windows extension interface, so decimal precision is limited to float precision for those values. Integer and string paths have separate typed marshalling.

### 3.4 File paths and storage

- Android file access is rooted at the app's private `Context.getFilesDir()` directory.
- Relative paths resolve beneath that root. Legacy Windows-drive paths (for example `C:\Sonic\Game.ini`) have the drive prefix removed and map beneath `filesDir`.
- Traversal (`..`), external absolute paths and symlink escapes outside the sandbox are refused before loading/saving, encrypted direct file I/O or directory creation.
- This does not reproduce arbitrary Windows filesystem access. Android scoped/private storage differences apply.
- Existing INI content, UTF-8 text and the shared encryption implementation are retained in the C++ core; device-level cross-platform save/load still needs testing.

## 4. Evidence matrix

| Check | Result |
| --- | --- |
| `python3 tools/gen_android_aces.py --check` | Passed locally; generated C++/Java metadata is up to date. |
| `bash tools/run_host_tests.sh` | Passed locally with GCC 12 on the current source. Covers v1 Windows-1252 and v2 Unicode edit data, sandbox path checks, representative ACE dispatch, and the guarded no-op for an object-property ACE whose object parameter is not marshalled. |
| `SANITIZE=1 bash tools/run_host_tests.sh` | Passed locally on the current source with AddressSanitizer + UBSan; same host-only coverage as above. |
| `bash tools/sync_workflow.sh --check` | Passed; `.github/workflows/main.yml` matches `ci/workflows/build.yml`. |
| NDK build of `libIniPlusPlusBridge.so` | Not run locally; no NDK installed. No Android `.so` artifact is currently verified. |
| Java compile against Fusion 295.10 | Not run; no `javac` and no matching 295.10 exporter source in the workspace. |
| Exporter registration/Gradle build | Not run successfully. The integration helper was run against the available 292.0 archive and correctly rejected it without changing files. |
| Windows Runtime/Edittime MFX build | Not run in this environment; requires Windows/MSVC. |
| Fusion export of the Sonic MFA to an APK | Not performed. |
| APK install and device test, including file compatibility and ACE behavior | Not performed. |

A passing host test is evidence for the paths it exercises, not evidence of an Android APK or a real Fusion exporter integration.
