# Ini++ (Unicode)

Ini++ is an INI-file extension for Clickteam Fusion 2.5 / MMF2 Unicode. Its existing Windows MFX, ACE definitions, editor data, icon and Windows editor resources remain the source of truth. This branch adds an Android runtime path without changing the Windows MFX ABI.

## Android status: source package, not a verified release

The Android implementation uses a Java `CRunExtension` adapter plus this project's JNI library, which calls the shared C++ ACE implementation. The Java sources target the Fusion 2.5 Java API surface present in the Fusion 293.0 runtime source archive; the intended deliverable is a source/ABI ZIP consumed from `<Fusion>/Data/Runtime/Android/INI++.zip` by the Fusion 295.10 exporter. The ZIP uses the runtime merge layout (`src/Extensions/` and `assets/mmf/<ABI>/`). It contains no precompiled `.class`, `.jar` or `.aar` dependency, no direct AndroidX imports, and no proprietary exporter files.

That source-compatible route is **not yet validated as a Fusion 295.10 extension**:

- The root `AndroidSDK_Gradle.zip` identifies as Fusion 293.0 (Gradle 7.5 / Android Gradle Plugin 7.4.2). It is an API reference only; it is not copied into the extension ZIP or used as the target build configuration.
- The Fusion 295.10 merge, class registration in its `CExtLoad` flow, Java/Gradle compile, unchanged Sonic MFA export and device test have not all been completed. A source API check is not a substitute for those tests.
- The current bridge preserves ACE IDs and parameter order, but it does not marshal Fusion object references or Ini++ custom-parameter payloads. Object-property ACEs are deliberately guarded rather than using fabricated pointers. See [Compatibility](docs/COMPATIBILITY.md).

Therefore do not call Android support complete or claim the Sonic MFA exports. Keep the existing MFA events unchanged for the acceptance test, but verify all ACEs that MFA actually uses before release.

## Separate AssArray Android subset for Sonic Classic 2

A Java adapter targets the five AssArray ACEs used by the supplied Sonic Classic 2 export: Set Value at Key, Clear, Load INI into Values, Save INI from Values, and `GetValue`. The checked-in `ringdata.sav` is a flat key/value sample from Sonic Classic 1; CI uses it only as a format fixture, not as evidence of Sonic Classic 2 save compatibility. `SC2sf.ini` remains opaque and is not used by the test. See [AssArray Android notes](docs/ASSARRAY_ANDROID.md) for implementation scope and test limits.

The root AssArray `.mfx` files are already-compiled binaries. There is no AssArray `.sln`, `.vcxproj`, or C/C++ source in this checkout, so a workflow cannot compile new AssArray MFXs yet. The CI workflow packages the Android subset; a Windows MFX compile job needs the AssArray source project first.

## Verification available now

```sh
python3 tools/gen_android_aces.py --check
bash tools/run_host_tests.sh
SANITIZE=1 bash tools/run_host_tests.sh
```

The C++ host tests cover shared ACE dispatch, current-group parameter order, UTF-8 results, numeric/packed-position values, edit-data payload handling, autosave and path traversal/symlink confinement. The Android CI job also compiles/tests the standalone AssArray INI store against the Sonic Classic 1 `ringdata.sav` format fixture and packages `ASSARRAY.zip`; this does **not** verify Sonic Classic 2 save compatibility. CI still does **not** compile the full Java adapter against Fusion 295.10, verify exporter registration, or prove APK/device loading.

## Build outline

```sh
# Windows MFX: Visual Studio 2022 / MSVC v143, Win32 developer prompt
msbuild INI++15.vcxproj /m /p:Configuration=Runtime  /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild INI++15.vcxproj /m /p:Configuration=Edittime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false

# Android native/JNI half: Android NDK r26.1.10909125 (C++20, ndk-build)
export ANDROID_NDK_HOME=/path/to/android-ndk-r26.1.10909125
tools/build_android.sh --out build/android

# Create the Fusion Android extension ZIP and optionally install it to the Fusion Data directory.
tools/package_android_extension.sh \
  --android-dir build/android \
  --out build/android-package \
  --install-to /path/to/Fusion/Data/Runtime/Android
```

The ZIP's intended name is `INI++.zip`. Fusion 295.10 still needs to merge/register/compile it using its own exporter toolchain. For direct Gradle project checks, `tools/integrate_android_exporter.sh` accepts Fusion 293.0 or 295.10 source trees only when the Java API member checks pass; the script's source checks are not a Java compile unless `--assemble-debug` is run.

Exact commands, Java/Gradle toolchain boundaries, compatibility notes and limitations are in [docs/BUILD.md](docs/BUILD.md), [docs/INSTALL.md](docs/INSTALL.md), [docs/GITHUB_ACTIONS.md](docs/GITHUB_ACTIONS.md) and [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md).

## Repository map

- `Runtime.cpp`, `ACEs.cpp`, `RunData.hpp`, `Menus.cpp`: shared extension runtime and ACE definitions.
- `Edittime.cpp`, `General.cpp`, `Properties.cpp`, `Extension.rc`, `Icon.bmp`: Windows editor/MFX resources; kept for existing MFAs.
- `android/java/`: Java runtime adapter and generated ACE parameter metadata.
- `android/jni/`: JNI bridge and NDK module setup.
- `android/runtime/`, `android/fusion/`: platform file/edit-data support, bridge and shared-source compatibility types.
- `android/tests/BridgeTests.cpp`: Java/JNI-independent host tests.
- `android/java/Extensions/CRunAssArray.java` and `AssArrayIniStore.java`: five-ACE Android compatibility adapter for Sonic Classic 2; the flat Sonic Classic 1 `ringdata.sav` format fixture is checked by `android/tests/AssArrayIniStoreTests.java` in CI.
- `tools/gen_android_aces.py`: generated parameter/dispatch tables from `Menus.cpp` and `ACEs.cpp`.
- `tools/package_android_extension.sh`: creates and verifies the Ini++ `Data/Runtime/Android` source/assets ZIP.
- `tools/package_assarray_android_extension.sh` and `tools/integrate_assarray_android_exporter.sh`: package/register the Java-only AssArray Android subset; neither builds Windows MFX files.
- `.github/workflows/main.yml`: CI entry point (mirrored from `ci/workflows/build.yml`); it builds Ini++ MFXs and Android artifacts, but cannot run Fusion's private exporter or export/test the Sonic MFA.

## Acceptance test still required

Install the generated `INI++.zip` under Fusion's `Data/Runtime/Android`, build the unchanged Sonic MFA with Fusion 2.5 build 295.10, inspect the APK's `assets/mmf/<ABI>/libIniPlusPlusBridge.so`, and test the ACEs and sandboxed file operations the game uses on a device. Verify the target exporter's `CExtLoad` registration and Java API compatibility; unchanged Java API assumptions alone do not establish compatibility. Until this passes, treat Android as work in progress.

## History

- Original Ini++ 1.x/2.x by Jax (Jamie McLaughlin): <https://github.com/LB--/Fusion-INI-plus-plus>
- Unicode rewrite and this fork: <https://github.com/mymes1/IniPlusPlus>
