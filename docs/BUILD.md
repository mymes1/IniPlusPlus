# Building Ini++

This repository retains the existing Fusion 2.5 Windows MFX and adds an Android Java/JNI source port. The Android code is **not yet a verified release**: host tests pass, but no NDK build, Java/Gradle compile against build 295.10, Fusion export, Sonic APK or device test has been completed in this environment.

## 1. Targets and exact dependencies

| Component | Target/version | Current verification |
| --- | --- | --- |
| Windows runtime and edittime MFX | Fusion 2.5 Unicode, Win32; Visual Studio 2022, MSVC v143, Windows SDK 10 | Reproducible MSBuild commands below; actual CI build still needed to validate this branch's final changes. |
| Android C++ shared library | NDK r26.1.10909125, `ndk-build`, C++20, Android API 21; ABIs `arm64-v8a`, `armeabi-v7a`, `x86_64` | Source/configuration only; NDK absent locally. CI builds this independent JNI/native half. |
| Android Java runtime integration | **Fusion 2.5 build 295.10**, the user's selected target | Blocked: the only Gradle archive at repo root reports **Fusion 292.0**, not 295.10. Do not silently use it. |
| Gradle/Android Gradle Plugin | Use the wrapper/plugin from the exact authorized 295.10 exporter project | Not yet known/validated. The repository's mismatched 292.0 archive declares Gradle 4.10.1 and AGP 3.3.1; those versions are recorded for identification only and are not certified for build 295.10. |
| Java/JDK for exporter build | Must be compatible with the exact 295.10 Gradle wrapper and Android Gradle Plugin | Unknown until the matching exporter project is supplied. Do not guess or upgrade its wrapper/plugin as a workaround. |
| Python | Python 3.9+ | Used by the ACE metadata generator/check. |
| Host C++ tests | GCC 11+ or Clang 14+, C++20 | Run with `tools/run_host_tests.sh`; locally verified with GCC 12. This is not an NDK/JNI/Java compile. |

### Confirmed archive mismatch

`AndroidSDK_Gradle.zip` is present at the repository root as a proprietary input. It is **not** copied into generated artifacts. Inspection of this exact archive gives:

- `app/src/main/java/Runtime/MMFRuntime.java`: `Fusion 292.0`
- `gradle/wrapper/gradle-wrapper.properties`: Gradle `4.10.1`
- root `build.gradle`: Android Gradle Plugin `3.3.1`

The requested exporter is build **295.10**. These are different versions. `tools/integrate_android_exporter.sh` explicitly checks the runtime source and fails before changing files unless it finds `Fusion 295.10`. Obtain the exact authorized 295.10 exporter project and its supported JDK/SDK/Gradle/AGP details before continuing. The source tree inside the 292.0 archive is not redistributed or used as a substitute.

## 2. Android architecture and boundaries

Android uses a Java `CRunExtension` adapter (`android/java/Extensions/CRunIniPlusPlus.java`) and a project-owned JNI shared library (`android/jni/JniBridge.cpp`). The runtime must explicitly register the Java class in `Extensions/CExtLoad.java`; the integration helper adds that registration to the **exact** 295.10 exporter project. The JNI library is copied under `app/src/main/assets/mmf/<ABI>/libIniPlusPlusBridge.so`, where the Gradle runtime's `MMFRuntime` loads non-`CRun*.so` assets with `System.load()`.

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
| `tools/gen_android_aces.py` | Generates/validates Java and C++ ACE parameter/dispatch tables. |
| `tools/build_android.sh` | NDK native build and JNI export checks. |
| `tools/integrate_android_exporter.sh` | Version-guards, copies adapter/native files and registers the class in the exact exporter source. |
| `tools/package_android_extension.sh` | Packages the overlay ZIP from actual NDK outputs; refuses missing ABI binaries by default. |
| `ci/workflows/build.yml` | Canonical CI; `.github/workflows/main.yml` is mirrored from it. |

## 4. Host checks (no Fusion/NDK/JDK required)

From the repository root:

```sh
python3 tools/gen_android_aces.py --check
bash tools/run_host_tests.sh
SANITIZE=1 bash tools/run_host_tests.sh
```

The script runs the generator check, compiles `BridgeTests.cpp`, `Bridge.cpp`, the shared runtime/ACEs, the Android path/file layer and Unicode utilities, then executes the tests. Current cases cover:

- generated ACE count, action/condition argument order and number/string/position dispatch;
- UTF-8 expression results, numeric return types and the packed X/Y `PARAM_POSITION` encoding;
- payload-only legacy v1 ANSI (Windows-1252) and v2 Unicode edit data used by the Java adapter;
- a safe no-op when an object-property ACE receives a placeholder instead of a marshalled Fusion object;
- autosave, Windows-drive path mapping to a filesDir-relative path, traversal rejection, outside absolute paths and symlink escapes.

These tests prove the tested C++ paths only. They do not compile `JniBridge.cpp`, invoke `javac`/Gradle, load `CExtLoad`, produce an APK or establish behavior in a proprietary runtime.

## 5. Windows MFX

Run from a Visual Studio 2022 developer prompt with the repository submodule initialized:

```bat
git submodule update --init --recursive
msbuild INI++15.vcxproj /m /p:Configuration=Runtime  /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild INI++15.vcxproj /m /p:Configuration=Edittime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
```

- `Runtime|Win32` builds `Build\Output\Runtime\INI++15.mfx` (install as `INI++.mfx` in `Data\Runtime\Unicode`).
- `Edittime|Win32` builds `Build\Output\Edittime\INI++15.mfx` (install as `INI++.mfx` in `Extensions\Unicode`).
- `PostBuildEventUseInBuild=false` disables the local Fusion auto-install post-build step.
- The Windows MFX has not been built as part of this continuation. Do not claim it was revalidated until the MSBuild/CI job passes.

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

## 7. Integrate with Fusion 2.5 build 295.10

The integration requires an **authorized, extracted Gradle exporter source tree whose `MMFRuntime.java` identifies as Fusion 295.10**. It refuses the checked-in 292.0 archive and other versions:

```sh
tools/integrate_android_exporter.sh \
  --exporter /path/to/extracted-Fusion-295.10-Gradle-project \
  --android-dir build/android \
  --check-only

# After the source checks pass, copy Java files/libs and patch CExtLoad.java:
tools/integrate_android_exporter.sh \
  --exporter /path/to/extracted-Fusion-295.10-Gradle-project \
  --android-dir build/android

# Optional Gradle compile/APK build using that project's own wrapper:
tools/integrate_android_exporter.sh \
  --exporter /path/to/extracted-Fusion-295.10-Gradle-project \
  --android-dir build/android \
  --assemble-debug
```

The helper prints the wrapper Gradle and Android Gradle Plugin versions read from the exporter, checks the Java API members the adapter calls, refuses to overwrite differing source/assets, copies the adapter and ABI libraries, and adds `CRunIniPlusPlus` to `CExtLoad.java`. `--assemble-debug` runs `./gradlew --no-daemon :app:assembleDebug` after integration and verifies an APK file was produced. It still does not export the user's MFA or test a device.

The helper has only been run against the repository's mismatched 292.0 archive to confirm the version guard rejects it without modifying files. The successful 295.10 path has **not** been exercised.

## 8. Fusion Android overlay ZIP and candidate packaging

After a successful NDK build, create the app-source/assets overlay ZIP:

```sh
tools/package_android_extension.sh --android-dir build/android --out build/android-package
```

It requires all three configured ABI libraries and creates `build/android-package/INI++.zip`. The ZIP contains Ini++-owned Java files, the built JNI `.so` assets, a small `CExtLoad` registration patch and a warning README. It contains **no Clickteam SDK/exporter source**. It is an overlay for the extracted Gradle project, not a verified Fusion `Data/Runtime/Android` merge or a tested installable extension. The actual exporter integration uses the version-guarded script above.

For source-layout debugging only, `--allow-incomplete` writes `INI++-INCOMPLETE.zip` and a `MISSING-BINARIES.txt` marker. `tools/package_release.sh` refuses that file and always stamps any release candidate as Android-unverified. It cannot produce a verified complete release by itself.

A full product release still requires real runtime artifacts plus the exact build 295.10 integration and the user's Sonic MFA APK/device acceptance test. Do not upload a host-test-only or source-only ZIP as a completed Android extension.

## 9. CI and artifact workflow

The canonical workflow is `.github/workflows/main.yml`; `ci/workflows/build.yml` is maintained as its mirror:

```sh
tools/sync_workflow.sh --check
```

CI is intended to run the generator/host tests (including ASan/UBSan), MSVC Win32 MFX builds, and the NDK JNI build, uploading **separate** Windows and Android-native/source artifacts. It cannot access the user's licensed 295.10 exporter source or Sonic MFA, so it must not claim a complete export or publish a verified release bundle. Workflow changes under `.github/workflows/` require the GitHub token used to push to have the workflow-scoped permission; if push is rejected for that reason, reconnect GitHub in Arena with that permission.

The proprietary `AndroidSDK_Gradle.zip` stays at the repository root as an input; it must not be copied into generated ZIPs or artifacts.

## 10. Troubleshooting

| Symptom | Meaning / next step |
| --- | --- |
| `ndk-build was not found` | Install NDK r26.1.10909125 and set `ANDROID_NDK_HOME`, or pass `--ndk`. |
| Integration reports `Fusion 292.0, not ... 295.10` | The supplied source tree is the wrong exporter version. Do not override the check; obtain the authorized 295.10 project. |
| Integration reports missing `CExtLoad` or Java API members | Exporter source layout/API differs. Stop and compare against the exact 295.10 API; do not patch in a different runtime silently. |
| `tools/package_android_extension.sh` reports missing ABI libraries | Run a real NDK build for the configured ABIs. An incomplete ZIP is source-only and not a release. |
| Gradle/JDK fails | Use the Java/Gradle/AGP versions supported by the exact 295.10 exporter, not those read from the mismatched 292.0 archive. Record them in the build log. |
| APK starts but Ini++ is missing | Confirm CExtLoad registers `CRunIniPlusPlus`, `libIniPlusPlusBridge.so` is in `assets/mmf/<device ABI>/`, and logcat has no JNI load/symbol errors. |
| Save/load is rejected | Paths are confined to the app's files directory. Legacy drive-form paths are mapped below `filesDir`; traversal and external paths are refused. |
| Windows build cannot find lSDK headers/libraries | Run `git submodule update --init --recursive` and build from the VS 2022 Win32 configuration. |
