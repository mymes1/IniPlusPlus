# Ini++ Android compatibility and verification status

## Status first

The Android path is an **unfinished source implementation**, not a completed Fusion extension release. It uses a Java `CRunExtension` adapter calling shared C++ ACE bodies through this project's JNI library. The Java source baseline is the Fusion 2.5 **293.0** API in the locally available exporter archive; the intended target remains Fusion 2.5 **295.10**, which should source-merge and compile the adapter. That 293-to-295.10 assumption is not yet validated by a target Gradle compile, `CExtLoad` registration, Fusion merge, Sonic MFA APK export or device test.

The root `AndroidSDK_Gradle.zip` identifies as Fusion 293.0 (Gradle 7.5 / AGP 7.4.2). It is used only to inspect Java API declarations. The source package contains no vendor runtime/exporter files, no JAR/AAR dependencies and no direct AndroidX imports. The target Fusion 295.10 exporter must provide its own Gradle/SDK configuration. Do not call Android support complete based only on the source ZIP or the version/API assumption.

## 1. Architecture

- `Runtime.cpp`, `ACEs.cpp`, `RunData.hpp`, `Settings.hpp` and the `Menus.cpp` ACE declarations are shared with the Windows MFX.
- `android/java/Extensions/CRunIniPlusPlus.java` implements the Java `CRunExtension` surface. It reads action/condition/expression parameters with the Java runtime's typed APIs and holds the object lifecycle.
- `android/jni/JniBridge.cpp` converts Java values to UTF-8/bridge input and returns `Expressions.CValue` results.
- `android/runtime/Bridge.cpp` owns the shared C++ run object and dispatches the existing ACE functions. `tools/gen_android_aces.py` derives the ordered Java/C++ parameter tables from `Menus.cpp` and the shared ACE tables.
- `android/runtime/AndroidRuntime.cpp` provides Android app-private file access, file I/O, logging and edit-data decoding. `safe_data_path()` checks paths before text I/O, encrypted direct I/O and parent-directory creation.
- The Android build uses JNI and Java registration, **not** Clickteam's historical native `RuntimeNative.h` ABI. The project's Android compatibility declarations are not a replacement Fusion runtime or C++ SDK.

`CExtLoad.java` in the generated runtime must register `CRunIniPlusPlus`; the class is not loaded by a compile define alone. `tools/package_android_extension.sh` creates a source ZIP in the Fusion merge layout (`src/Extensions/` and `assets/mmf/`). It does not contain or overwrite the proprietary loader. The optional `tools/integrate_android_exporter.sh` checks the API members and patches the extracted target project's loader without replacing the file. Whether Fusion 295.10 automatically adds this registration while merging an installed `Data/Runtime/Android/INI++.zip` still needs an export test.

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
| `bash tools/run_host_tests.sh` | Passed locally with GCC 12 on the latest source, including C++ bridge tests and the extension ZIP layout/install smoke test. The latter uses dummy, non-runnable `.so` files. |
| `SANITIZE=1 bash tools/run_host_tests.sh` | Passed locally with AddressSanitizer + UBSan on the latest source; same host-only coverage. |
| Android extension ZIP smoke test | Passed locally; checks the Clickteam `src/Extensions` + `assets/mmf` paths, install destination and absence of JAR/AAR/vendor files using dummy libraries only. Not a Fusion merge test. |
| `bash tools/sync_workflow.sh --check` | Passed locally; `.github/workflows/main.yml` matches `ci/workflows/build.yml`. |
| Fusion 293.0 Java API source check | Passed against the available archive with `tools/integrate_android_exporter.sh --check-only` and a test-only placeholder `.so`. This is not `javac` or Gradle. |
| CI run 38041243939 | Host tests passed; Android native build and Windows MFX jobs failed. Android SDK/NDK installation succeeded. The remaining compiler diagnostics are not available in this workspace. |
| NDK build of `libIniPlusPlusBridge.so` | CI attempted but no passing post-fix build result is available; local NDK is absent. No verified Android `.so` artifact is established. |
| Java compile against Fusion 295.10 | Not run; no local `javac` or 295.10 exporter source. The 293.0 API checks are not a 295.10 compile. |
| Exporter ZIP merge/registration/Gradle build | Not validated. The `Data/Runtime/Android` source-package merge and target `CExtLoad` registration need an actual Fusion 295.10 export test. |
| Windows Runtime/Edittime MFX build | CI attempted but latest MFX job failed; a successful post-fix run is still required. |
| Fusion export of the Sonic MFA to an APK | Not performed. |
| APK install and device test, including file compatibility and ACE behavior | Not performed. |

A passing host test is evidence for the paths it exercises, not evidence of an Android APK or a real Fusion exporter integration.
