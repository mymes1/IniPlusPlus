# Building Ini++

This repository retains Ini++'s existing Fusion 2.5 Windows MFX, adds its Android Java/JNI source port, and authors the separate AssArraySC2 replacement object with its own Windows MFX project and Android Java runtime. The new MFX uses independent sources and the `lSDK` submodule; it does not require original AssArray source. Android exporter merge/Java compilation, target MFA export and device tests remain unverified.

## 1. Targets and exact dependencies

| Component | Target/version | Current verification |
| --- | --- | --- |
| Windows runtime and edittime MFX | Fusion 2.5 Unicode, Win32; Visual Studio 2022, MSVC v143, Windows SDK 10 | Two projects: existing Ini++ and independent AssArraySC2 replacement. CI is configured to build both MFX pairs and check exports; its first run remains pending. |
| Android C++ shared library | NDK r26.1.10909125, `ndk-build`, C++20, Android API 21; ABIs `arm64-v8a`, `armeabi-v7a`, `x86_64` | Host bridge tests pass. The pinned NDK is installed in CI, but a passing post-fix native build is still required. |
| Android Java runtime integration | Fusion 2.5 build **295.10** target; Java source API baseline from Fusion **293.0** | Source ZIP is implemented; Fusion 295.10 merge, registration, Java/Gradle compile, MFA export and device run are not yet verified. |
| Gradle/Android Gradle Plugin | Use Fusion 295.10 exporter's own Gradle wrapper and plugin | The 293.0 reference archive reports Gradle 7.5 / AGP 7.4.2. Do not copy those project settings into the target exporter or assume they prove 295.10 compatibility. |
| Java/JDK for exporter build | Use the JDK supported by the target Fusion 295.10 exporter | No local JDK/`javac`; exact exporter toolchain compile remains pending. |
| Python | Python 3.9+ | Used by the ACE metadata generator/check. |
| Host C++ tests | GCC 11+ or Clang 14+, C++20 | Run with `tools/run_host_tests.sh`; host bridge and package-layout tests run locally. This is not an NDK/JNI/Java compile. |

### Fusion 293.0 Java API baseline and Fusion 295.10 target

`AndroidSDK_Gradle.zip` is present locally at the repository root as a proprietary input. It reports `Fusion 293.0` in `app/src/main/java/Runtime/MMFRuntime.java`, Gradle `7.5`, and Android Gradle Plugin `7.4.2`. We use the archive only to inspect the Java `CRunExtension`/`CBinaryFile` API surface; it is ignored by Git and is never copied into generated artifacts.

The Android extension package is built from Ini++-owned Java source and NDK libraries, not from the 293.0 Gradle project. It uses Clickteam's documented runtime-merge ZIP layout (`src/Extensions/` and `assets/mmf/<ABI>/`) and is intended to be installed as `<Fusion>/Data/Runtime/Android/INI++.zip` ([Clickteam Android SDK guide](https://github.com/ClickteamLLC/android/blob/master/docs/index.md#packaging)). Fusion 295.10 is the target exporter and should own the Gradle, AndroidX, SDK and target-SDK configuration. The 293.0 archive is an API reference—not the 295.10 exporter, dependency bundle or build configuration.

The user-provided compatibility assumption is that the `CRun`/`CExtension`/`CBinaryFile` Java runtime API did not change between 293.0 and 295.10. `tools/integrate_android_exporter.sh` therefore checks API members and accepts source trees labelled 293.0 or 295.10. This static source guard does not prove source-zip merging, registration, Java compilation or runtime compatibility in 295.10; the target exporter test remains required.

## 2. Android architecture and boundaries

Android uses a Java `CRunExtension` adapter (`android/java/Extensions/CRunIniPlusPlus.java`) and a project-owned JNI shared library (`android/jni/JniBridge.cpp`). The extension ZIP carries sources under `src/Extensions/` and libraries under `assets/mmf/<ABI>/`, as expected by the Fusion Android runtime merge convention. The adapter must also be registered in the target exporter's `Extensions/CExtLoad.java` flow; the optional direct-project helper can insert that branch without replacing the exporter file. The Gradle runtime loads non-`CRun*.so` assets with `System.load()`.

The JNI route does not include or recreate Clickteam's old native extension ABI, and it does not require `RuntimeNative.h`. `android/fusion/` provides only the types/helpers needed to compile the shared Ini++ C++ implementation and run host tests; those declarations are not a substitute for exporter integration. The generated Java parameter table, C++ dispatch table and declarations come from `Menus.cpp` and `ACEs.cpp` through `tools/gen_android_aces.py`.

This is source-level integration work, not proof that the class registration, JNI name resolution, asset merge or data pointer convention matches build 295.10. See [COMPATIBILITY.md](COMPATIBILITY.md) for Android differences, including current object/custom-parameter limitations.

## 3. Source layout

| Path | Purpose |
| --- | --- |
| `Runtime.cpp`, `ACEs.cpp`, `RunData.hpp`, `Settings.hpp`, `CustomParams.*`, `Menus.cpp` | Shared runtime and ACE definitions; Windows and Android use the same ACE IDs/order. |
| `Edittime.cpp`, `General.cpp`, `Properties.cpp`, `Extension.rc`, `Icon.bmp` | Existing Windows editor/MFX resources and serialized edit-data definitions. |
| `android/java/Extensions/` | Java `CRunExtension` adapter plus generated parameter metadata. |
| `android/jni/JniBridge.cpp`, `Android.mk`, `Application.mk` | JNI conversion and native NDK module. |
| `android/runtime/Bridge.cpp`, `Bridge.hpp`, `AndroidRuntime.*` | Shared ACE dispatch, edit data, sandbox paths and Android text-file I/O. |
| `android/fusion/` | Source adaptation types/helper declarations used by the shared C++ code. Not a Clickteam ABI. |
| `android/tests/BridgeTests.cpp` | Host tests; does not compile JNI or Java. |
| `extensions/AssArraySC2/` | Independent Fusion 2.5 Win32 editor/runtime MFX project, ACE metadata, numeric flat-INI store and host tests for the Replace Object target. |
| `android/java/Extensions/CRunAssArraySC2.java`, `AssArraySC2IniStore.java` | Android runtime for the separate `ASSARRAYSC2` identity; the Sonic Classic 1 format fixture is tested in `android/tests/AssArraySC2IniStoreTests.java`. |
| `tools/check_assarray_sc2_extension.py` | Static guard for five ACE IDs/signatures and distinct Windows/Android identity. |
| `tools/package_assarray_sc2_android_extension.sh`, `tools/integrate_assarray_sc2_android_exporter.sh` | Package/register `ASSARRAYSC2.zip` without replacing Fusion's vendor loader. |
| `tools/gen_android_aces.py` | Generates/validates Java and C++ ACE parameter/dispatch tables. |
| `tools/build_android.sh` | NDK native build and JNI export checks. |
| `tools/integrate_android_exporter.sh` | Optional direct-project source/API check and integration for Ini++ against Fusion 293.0 or 295.10. |
| `tools/package_android_extension.sh` | Packages/verifies the Ini++ `Data/Runtime/Android` ZIP (`src/Extensions`, `assets/mmf`); refuses missing ABI binaries by default and rejects JAR/AAR/class files. |
| `ci/workflows/build.yml` | Canonical workflow mirrored to `.github/workflows/main.yml`; configured to build Ini++ and AssArraySC2 MFXs and package Android artifacts. |

## 4. Host checks (no Fusion/NDK/JDK required)

From the repository root:

```sh
python3 tools/gen_android_aces.py --check
bash tools/run_host_tests.sh
SANITIZE=1 bash tools/run_host_tests.sh
```

The script checks generated ACE metadata and AssArraySC2 signatures, compiles/runs Ini++ `BridgeTests.cpp` and the separate AssArraySC2 numeric-store tests, then smoke-tests both Android package layouts. The Ini++ package test uses placeholder `.so` files. Current cases cover:

- generated ACE count, action/condition argument order and number/string/position dispatch;
- UTF-8 expression results, numeric return types and the packed X/Y `PARAM_POSITION` encoding;
- payload-only legacy v1 ANSI (Windows-1252) and v2 Unicode edit data used by the Java adapter;
- a safe no-op when an object-property ACE receives a placeholder instead of a marshalled Fusion object;
- autosave, Windows-drive path mapping to a filesDir-relative path, traversal rejection, outside absolute paths and symlink escapes;
- the Fusion `src/Extensions` + `assets/mmf` ZIP layout, install destination, and exclusion of `.class`/`.jar`/`.aar` and proprietary exporter files (using placeholder, non-runnable `.so` files);
- the ASSARRAYSC2 source-package identity/layout and exclusion of original AssArray artifacts;
- numeric store parsing, ASCII key-case normalization, UTF-8 round-trip, bad-file rejection and valid empty-file behavior.

These host checks do not build the Windows MFX or compile `JniBridge.cpp`, validate Fusion 295.10's merge/`CExtLoad` behavior, produce a real APK, test Replace Object on the target MFA or establish SC2 save compatibility. The store fixture is from Sonic Classic 1 only.

The Android CI job is configured to use JDK 17 to test the standalone AssArraySC2 numeric store against the flat Sonic Classic 1 `ringdata.sav` fixture. This does not test a Sonic Classic 2 save or establish target-game compatibility:

```sh
mkdir -p build/assarraysc2-tests
javac -encoding UTF-8 -d build/assarraysc2-tests \
  android/java/Extensions/AssArraySC2IniStore.java \
  android/tests/AssArraySC2IniStoreTests.java
java -cp build/assarraysc2-tests Extensions.AssArraySC2IniStoreTests ringdata.sav
```

This test does not compile `CRunAssArraySC2.java` against Fusion's private Java classes. No local JDK/`javac` is available in this environment.

## 5. Windows MFX

Run from a Visual Studio 2022 developer prompt with the repository submodule initialized:

```bat
git submodule update --init --recursive
msbuild INI++15.vcxproj /m /p:Configuration=Runtime  /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild INI++15.vcxproj /m /p:Configuration=Edittime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild extensions/AssArraySC2/AssArraySC2.vcxproj /m /p:Configuration=Runtime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild extensions/AssArraySC2/AssArraySC2.vcxproj /m /p:Configuration=Edittime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
```

- `Runtime|Win32` builds `Build\Output\Runtime\INI++15.mfx` (install as `INI++.mfx` in `Data\Runtime\Unicode`).
- `Edittime|Win32` builds `Build\Output\Edittime\INI++15.mfx` (install as `INI++.mfx` in `Extensions\Unicode`).
- AssArraySC2 outputs `build\mfx\AssArraySC2\Runtime\AssArraySC2.mfx` and `build\mfx\AssArraySC2\Edittime\AssArraySC2.mfx`; install to `<Fusion>\Data\Runtime\Unicode` and `<Fusion>\Extensions\Unicode`, respectively.
- The new project uses the initialized `lSDK` submodule and owns its sources/resources; it does not include or rebuild the original prebuilt AssArray MFX files and does not require original source.
- `PostBuildEventUseInBuild=false` disables local Fusion auto-install. Windows CI is configured to build both MFX projects and check exports; the first run remains pending.

## 6. Android native/JNI build

Install Android NDK `26.1.10909125`, then:

```sh
export ANDROID_NDK_HOME=/path/to/android-ndk-r26.1.10909125
python3 tools/gen_android_aces.py --check
tools/build_android.sh --out build/android
```

The module compiles `Bridge.cpp`, `JniBridge.cpp`, `Runtime.cpp`, `ACEs.cpp`, `AndroidRuntime.cpp` and `UnicodeUtilities.cpp`. `Application.mk` sets `APP_PLATFORM=android-21`, `APP_STL=c++_static`, C++20 exceptions/RTTI and the three ABIs. `Android.mk` links Android logging and uses 16 KiB max-page alignment. Output:

```text
build/android/assets/mmf/arm64-v8a/libIniPlusPlusBridge.so
build/android/assets/mmf/armeabi-v7a/libIniPlusPlusBridge.so
build/android/assets/mmf/x86_64/libIniPlusPlusBridge.so
```

The script checks the JNI exports for all seven Java native methods. It does not validate that the classes compile in the selected exporter or that the runtime can load the library. `tools/build_android.sh --check-only` checks generated tables only; it is not a native build.

## 7. Package for Fusion's Android runtime merge

After a successful NDK build, create the source/assets ZIP in the standard Fusion Android extension layout:

```sh
tools/package_android_extension.sh \
  --android-dir build/android \
  --out build/android-package

# Optional: install the completed ZIP into a local Fusion installation.
tools/package_android_extension.sh \
  --android-dir build/android \
  --out build/android-package \
  --install-to /path/to/Fusion/Data/Runtime/Android
```

The resulting `build/android-package/INI++.zip` has this layout:

```text
src/Extensions/CRunIniPlusPlus.java
src/Extensions/IniPlusPlusAceParams.java
assets/mmf/arm64-v8a/libIniPlusPlusBridge.so
assets/mmf/armeabi-v7a/libIniPlusPlusBridge.so
assets/mmf/x86_64/libIniPlusPlusBridge.so
```

This is raw Java source, not a precompiled `.class`/`.jar`/`.aar` extension. It has no direct AndroidX imports and does not bundle the root `AndroidSDK_Gradle.zip` or any vendor exporter/runtime files. Fusion 295.10 is expected to merge these files and compile them with its own Gradle/SDK/target-SDK setup. The package script verifies the archive paths and refuses JAR/AAR/class or proprietary exporter entries. A package check with placeholder `.so` files is not a native build or runtime test.

The ZIP does not contain a copied vendor `CExtLoad.java`. The adapter must be registered in the target exporter's loader flow. The packager writes `CExtLoad-registration.patch` next to the ZIP as a manual reference; it is not a complete loader and is not applied by the package builder. Whether Fusion 295.10 automatically performs this registration from the installed ZIP must be verified during export.

The optional direct-project helper can check and integrate against an extracted Fusion 293.0 or 295.10 Gradle project:

```sh
tools/integrate_android_exporter.sh \
  --exporter /path/to/extracted-Fusion-295.10-project \
  --android-dir build/android \
  --check-only

tools/integrate_android_exporter.sh \
  --exporter /path/to/extracted-Fusion-295.10-project \
  --android-dir build/android \
  --assemble-debug
```

The helper checks only the Java API members used by the adapter, copies files into that project's `app/src/main` tree, and inserts a `CExtLoad` branch. With `--assemble-debug`, it runs that exporter's own Gradle wrapper. A Java source guard is not a compile; a successful Gradle build still does not export the user's MFA or test a device.

For source-layout debugging only, `--allow-incomplete` writes `INI++-INCOMPLETE.zip` and a `MISSING-BINARIES.txt` marker. It refuses installation. `tools/package_release.sh` rejects an incomplete archive and always stamps any release candidate as Android-unverified.

A full product release still requires the real NDK build, Fusion 295.10 merge/registration/compile, the user's unchanged Sonic MFA APK export and device acceptance test. Do not upload a package-only ZIP as a completed Android extension.

## 9. CI and artifact workflow

The canonical workflow is `.github/workflows/main.yml`; `ci/workflows/build.yml` is maintained as its mirror:

```sh
tools/sync_workflow.sh --check
```

The configured workflow is intended to run generator/host tests (including ASan/UBSan), build Ini++ and AssArraySC2 MSVC Win32 MFX pairs, and build Android JNI libraries. Android CI tests the standalone Java store against the Sonic Classic 1 fixture and packages `ASSARRAYSC2.zip`; it does not compile the full adapter against Fusion, export the target MFA, or test an SC2 save/device. No CI run for this branch has been verified yet. The workflow is configured to upload separate Windows and Android artifacts and cannot access the licensed 295.10 exporter or Sonic MFA. Workflow changes under `.github/workflows/` require the GitHub token used to push to have the workflow-scoped permission; if push is rejected for that reason, reconnect GitHub in Arena with that permission.

The proprietary `AndroidSDK_Gradle.zip` stays at the repository root as an input; it must not be copied into generated ZIPs or artifacts. For click-by-click workflow triggering, artifact download, and Windows/Android integration steps, see [GITHUB_ACTIONS.md](GITHUB_ACTIONS.md).

## 10. Troubleshooting

| Symptom | Meaning / next step |
| --- | --- |
| `ndk-build was not found` | Install NDK r26.1.10909125 and set `ANDROID_NDK_HOME`, or pass `--ndk`. |
| Integration rejects an exporter version | Each direct-project helper accepts only inspected Fusion 293.0/295.10 labels and checks the API surface. Use the target exporter and inspect the reported missing member; do not bypass the guard. |
| Integration reports missing `CExtLoad` or Java API members | Exporter source layout/API differs. Stop and compare the actual target API; do not copy/replace a vendor loader from another version. |
| `tools/package_android_extension.sh` reports missing ABI libraries | Run a real NDK build for the configured ABIs. An incomplete ZIP is source-only, marked incomplete and cannot be installed. |
| Gradle/JDK fails | Use the Java/Gradle/AGP versions supplied by the target Fusion 295.10 exporter. The 293.0 reference project's configuration is not copied to the package. |
| APK starts but Ini++ is missing | Confirm CExtLoad registers `CRunIniPlusPlus`, `libIniPlusPlusBridge.so` is in `assets/mmf/<device ABI>/`, and logcat has no JNI load/symbol errors. |
| Save/load is rejected | Paths are confined to the app's files directory. Legacy drive-form paths are mapped below `filesDir`; traversal and external paths are refused. |
| Windows build cannot find lSDK headers/libraries | Run `git submodule update --init --recursive` and build from the VS 2022 Win32 configuration. |
