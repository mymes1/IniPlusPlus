# Ini++ (Unicode)

Ini++ is an INI-file extension for Clickteam Fusion 2.5 / MMF2 Unicode. Its existing Windows MFX, ACE definitions, editor data, icon and Windows editor resources remain the source of truth. This branch adds an Android runtime path without changing the Windows MFX ABI.

## Android status: source package, not a verified release

The Android implementation uses a Java `CRunExtension` adapter plus this project's JNI library, which calls the shared C++ ACE implementation. The Java sources target the Fusion 2.5 Java API surface present in the Fusion 293.0 runtime source archive; the intended deliverable is a source/ABI ZIP consumed from `<Fusion>/Data/Runtime/Android/INI++.zip` by the Fusion 295.10 exporter. The ZIP uses the runtime merge layout (`src/Extensions/` and `assets/mmf/<ABI>/`). It contains no precompiled `.class`, `.jar` or `.aar` dependency, no direct AndroidX imports, and no proprietary exporter files.

That source-compatible route is **not yet validated as a Fusion 295.10 extension**:

- The root `AndroidSDK_Gradle.zip` identifies as Fusion 293.0 (Gradle 7.5 / Android Gradle Plugin 7.4.2). It is an API reference only; it is not copied into the extension ZIP or used as the target build configuration.
- The Fusion 295.10 merge, class registration in its `CExtLoad` flow, Java/Gradle compile, unchanged Sonic MFA export and device test have not all been completed. A source API check is not a substitute for those tests.
- The current bridge preserves ACE IDs and parameter order, but it does not marshal Fusion object references or Ini++ custom-parameter payloads. Object-property ACEs are deliberately guarded rather than using fabricated pointers. See [Compatibility](docs/COMPATIBILITY.md).

Therefore do not call Android support complete or claim the Sonic MFA exports. Keep the existing MFA events unchanged for the acceptance test, but verify all ACEs that MFA actually uses before release.

## Separate AssArraySC2 replacement object for Sonic Classic 2

This branch authors a **new, editor-visible Fusion extension object** named **AssArray SC2 Compatible** (`ASSARRAYSC2`), with its own Win32 editor/runtime MFX project and Android Java runtime. It is intended for Fusion's Replace Object workflow and keeps the five ACEs used by the Sonic Classic 2 event set at their compatible IDs/signatures: Action 0 Set Value at Key, Action 3 Clear the Array, Action 12 Load INI File into Values, Action 14 Save INI File from Values, and Expression 0 `GetValue`. It does not register as or replace the original `ASSARRAY` extension, does not modify/require original AssArray source, and does not add the SC1-only ACE set.

`ringdata.sav` is a flat key/value fixture from Sonic Classic 1; CI uses it only to exercise numeric INI parsing/round-tripping, not as evidence of Sonic Classic 2 save compatibility. `SC2sf.ini` remains opaque. No target-game save compatibility is claimed until a Sonic Classic 2 sample is generated/located and tested. See [AssArraySC2 notes](docs/ASSARRAY_SC2.md) for ACE mappings, MFX build/install, Android packaging, and verification limits.

The root `AssArray.mfx` and `AssArrayruntime.mfx` files remain the original prebuilt binaries; the new project's own sources/resources live under `extensions/AssArraySC2/` and use the initialized `lSDK` submodule, not those binaries. CI is configured to build the new MFX pair and package its Android runtime under the distinct name `ASSARRAYSC2.zip`; no CI/MSVC run has yet verified that build.

## Verification available now

```sh
python3 tools/gen_android_aces.py --check
bash tools/run_host_tests.sh
SANITIZE=1 bash tools/run_host_tests.sh
```

The C++ host tests cover Ini++ dispatch and sandbox behavior plus the independent AssArraySC2 numeric store; the latter is tested against the Sonic Classic 1 `ringdata.sav` format fixture only. The Windows CI job is configured to build the new AssArraySC2 editor/runtime MFX pair, but its first run remains pending. Android CI is configured to compile/test the Java store and package `ASSARRAYSC2.zip`; it does **not** compile the full adapter against Fusion 295.10, export the target MFA, or prove device loading/save compatibility.

## Build outline

```sh
# Windows MFX: Visual Studio 2022 / MSVC v143, Win32 developer prompt
msbuild INI++15.vcxproj /m /p:Configuration=Runtime  /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild INI++15.vcxproj /m /p:Configuration=Edittime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false

git submodule update --init --recursive
msbuild extensions/AssArraySC2/AssArraySC2.vcxproj /m /p:Configuration=Runtime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild extensions/AssArraySC2/AssArraySC2.vcxproj /m /p:Configuration=Edittime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false

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
- `extensions/AssArraySC2/`: independent Fusion 2.5 Win32 editor/runtime MFX project, ACE metadata, numeric flat-INI store, resources, and host tests for the Replace Object target.
- `android/java/Extensions/CRunAssArraySC2.java` and `AssArraySC2IniStore.java`: Android runtime for the separate `ASSARRAYSC2` identity; the SC1 format fixture is tested in `android/tests/AssArraySC2IniStoreTests.java`.
- `tools/gen_android_aces.py`: generated parameter/dispatch tables from Ini++ `Menus.cpp` and `ACEs.cpp`.
- `tools/check_assarray_sc2_extension.py`: static guard for ACE IDs/signatures and distinct Windows/Android object identity.
- `tools/package_assarray_sc2_android_extension.sh` and `tools/integrate_assarray_sc2_android_exporter.sh`: package/register `ASSARRAYSC2.zip` without replacing Fusion's vendor loader.
- `.github/workflows/main.yml`: CI entry point mirrored from `ci/workflows/build.yml`; it is configured to build both Windows MFX pairs and Android artifacts but cannot run the licensed Fusion exporter or test the Sonic MFA.

## Acceptance test still required

Install the AssArraySC2 Edittime MFX in `<Fusion>/Extensions/Unicode/` and its Runtime MFX in `<Fusion>/Data/Runtime/Unicode/`; on a copy of the Sonic MFA, use Fusion's Replace Object workflow to select **AssArray SC2 Compatible** and confirm the five existing SC2 events retain their IDs/parameters without manual conversion. Then install `ASSARRAYSC2.zip` under `<Fusion>/Data/Runtime/Android/`, export the replaced MFA with Fusion 2.5 build 295.10, verify `CExtLoad` registration, and test on a device. Generate or locate a real SC2 save by running the target game's existing Save INI event at its configured path, and test load/save/GetValue against that actual file before claiming target-game save compatibility. The Sonic Classic 1 `ringdata.sav` fixture does not satisfy this acceptance test.

## History

- Original Ini++ 1.x/2.x by Jax (Jamie McLaughlin): <https://github.com/LB--/Fusion-INI-plus-plus>
- Unicode rewrite and this fork: <https://github.com/mymes1/IniPlusPlus>
