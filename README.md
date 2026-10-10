# Ini++ (Unicode)

Ini++ is an INI-file extension for Clickteam Fusion 2.5 / MMF2 Unicode. Its existing Windows MFX, ACE definitions, editor data, icon and Windows editor resources remain the source of truth. This branch adds an Android runtime path without changing the Windows MFX ABI.

## Android status: source work, not a verified release

The Android implementation now uses a **Java `CRunExtension` adapter plus this project's JNI library**, which calls the shared C++ ACE implementation. It does not use or imitate Clickteam's legacy `RuntimeNative.h` ABI. The action/condition/expression parameter metadata is generated from `Menus.cpp`; host tests exercise the C++ bridge and app-sandbox file access.

Android support is **not complete or validated as a Fusion extension yet**:

- The requested target is Fusion 2.5 build **295.10**.
- The repository-root `AndroidSDK_Gradle.zip` currently reports `Fusion 292.0` in `MMFRuntime.java` (Gradle 4.10.1, Android Gradle Plugin 3.3.1). The integration helper refuses this mismatch; it is not silently used as a 295.10 substitute and is never copied into a runtime ZIP.
- This environment has no Java compiler, Android NDK, compatible 295.10 exporter source, or Sonic MFA/device. The NDK build, Java/Gradle compile, CExtLoad registration in build 295.10, Fusion merge/export and APK/device acceptance test have not run.
- The current bridge preserves ACE IDs and parameter order, but it does not yet marshal Fusion object references or Ini++ custom-parameter payloads. Object-property ACEs are deliberately guarded rather than using fabricated pointers. See [Compatibility](docs/COMPATIBILITY.md).

Therefore there is no complete, tested Android extension ZIP or claim that the Sonic game exports. The source-level changes and the exact remaining blockers are documented below.

## Verification available now

```sh
python3 tools/gen_android_aces.py --check
bash tools/run_host_tests.sh
SANITIZE=1 bash tools/run_host_tests.sh
```

The host tests cover shared ACE dispatch, current-group parameter order, UTF-8 results, numeric/packed-position values, edit-data payload handling, autosave and path traversal/symlink confinement. They do **not** compile JNI/Java or prove exporter/APK loading.

## Build outline

```sh
# Windows MFX: Visual Studio 2022 / MSVC v143, Win32 developer prompt
msbuild INI++15.vcxproj /m /p:Configuration=Runtime  /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild INI++15.vcxproj /m /p:Configuration=Edittime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false

# Android native/JNI half: Android NDK r26.1.10909125 (C++20, ndk-build)
export ANDROID_NDK_HOME=/path/to/android-ndk-r26.1.10909125
tools/build_android.sh --out build/android

# This requires the exact authorized Fusion 2.5 build 295.10 Gradle exporter project.
# It copies Java sources/libs, patches CExtLoad.java and rejects other runtime versions.
tools/integrate_android_exporter.sh \
  --exporter /path/to/Fusion-295.10-Gradle-project \
  --android-dir build/android \
  --assemble-debug
```

Exact commands, Java/Gradle version discovery, compatibility notes and limitations are in [docs/BUILD.md](docs/BUILD.md), [docs/INSTALL.md](docs/INSTALL.md), [docs/GITHUB_ACTIONS.md](docs/GITHUB_ACTIONS.md) and [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md).

## Repository map

- `Runtime.cpp`, `ACEs.cpp`, `RunData.hpp`, `Menus.cpp`: shared extension runtime and ACE definitions.
- `Edittime.cpp`, `General.cpp`, `Properties.cpp`, `Extension.rc`, `Icon.bmp`: Windows editor/MFX resources; kept for existing MFAs.
- `android/java/`: Java runtime adapter and generated ACE parameter metadata.
- `android/jni/`: JNI bridge and NDK module setup.
- `android/runtime/`, `android/fusion/`: platform file/edit-data support, bridge and shared-source compatibility types.
- `android/tests/BridgeTests.cpp`: Java/JNI-independent host tests.
- `tools/gen_android_aces.py`: generated parameter/dispatch tables from `Menus.cpp` and `ACEs.cpp`.
- `.github/workflows/main.yml`: CI entry point (mirrored from `ci/workflows/build.yml`); CI builds host tests, Windows MFX and Android JNI libraries, but cannot perform the user's private 295.10 exporter/Sonic APK acceptance test.

## Acceptance test still required

With the correct Fusion 2.5 build 295.10 exporter sources available, integrate the adapter, compile the Gradle project, install the unchanged Sonic MFA's Windows editor/runtime MFX as usual, export that normal MFA without event changes, inspect the APK's `assets/mmf/<ABI>/libIniPlusPlusBridge.so`, and run file load/save and representative ACEs on a device. Until that passes, treat Android as work in progress.

## History

- Original Ini++ 1.x/2.x by Jax (Jamie McLaughlin): <https://github.com/LB--/Fusion-INI-plus-plus>
- Unicode rewrite and this fork: <https://github.com/mymes1/IniPlusPlus>
