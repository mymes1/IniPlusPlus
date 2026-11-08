# Build and installation guide using GitHub Actions

## Important status: Android is not a verified install yet

CI run [42492164267](https://github.com/mymes1/IniPlusPlus/actions/runs/42492164267) passed all jobs: shared-C++ host/sanitizer tests, Ini++ and AssArraySC2 Windows MFX builds, the standalone AssArraySC2 numeric INI store test against the Sonic Classic 1 `ringdata.sav` fixture, Android JNI builds, and ZIP packaging. This does **not** compile the full Android adapter with the Fusion 2.5 build 295.10 exporter, test an SC2 save, export the Sonic MFA, or produce a device-tested APK. The Android ZIP remains **unverified in the target exporter**.

The repository's `AndroidSDK_Gradle.zip` identifies as Fusion 293.0 and is used only as a Java API reference. Android packages are designed for the Fusion 295.10 exporter to merge and compile source; its Gradle/SDK configuration and AndroidX dependencies are not bundled. Ini++ object references/custom-parameter payloads also remain unimplemented; see [COMPATIBILITY.md](COMPATIBILITY.md). Do not treat a green workflow run as proof that Android support is complete.

## 1. Make the workflow available on GitHub

The workflow is `.github/workflows/main.yml`; its canonical mirror is `ci/workflows/build.yml`. It runs on pushes, pull requests and manual `workflow_dispatch` runs.

- A GitHub connection used to push workflow changes must be authorized to update `.github/workflows/`; this is a GitHub permission, not a secret to put in the repository.
- Do not upload `AndroidSDK_Gradle.zip`, the licensed Fusion exporter, or the Sonic MFA to GitHub Actions artifacts, runtime extension ZIPs or release bundles.
- A push to a branch containing the workflow starts it automatically. For **Run workflow** (`workflow_dispatch`), GitHub requires the workflow file to exist on the repository's default branch; until then, use a push-triggered run or merge it first.

Before pushing workflow changes, verify the two YAML files match:

```sh
bash tools/sync_workflow.sh --check
```

## 2. Jobs and artifacts

A successful run has these three jobs:

| Job | What it checks/builds | Artifact |
| --- | --- | --- |
| `host-tests` | Generated ACE metadata, ASSARRAYSC2 ID/signature checks, C++ bridge/store/package tests, ASan/UBSan and workflow mirror | None |
| `android-jni` | Ubuntu NDK r26.1.10909125 build for `arm64-v8a`, `armeabi-v7a`, and `x86_64`; standalone store test against the SC1 fixture; Ini++ and ASSARRAYSC2 Android ZIP packaging | `android-jni-overlay-unverified` |
| `windows-mfx` | Ini++ and AssArraySC2 Runtime/Edittime MFX pairs using MSVC v143/Win32; required export checks | `windows-mfx` |

The Android job uses JDK 17 for the standalone store test; it does **not** compile `CRunAssArraySC2` against Fusion classes or run the Fusion Gradle exporter. It has no target SC2 save or MFA. A green job does not validate ZIP ingestion, `CExtLoad` registration, Replace Object behavior, target MFA export or device/save compatibility.

## 3. Download artifacts

1. Open the successful workflow run in **Actions**.
2. At the bottom of the run summary, download `windows-mfx` or `android-jni-overlay-unverified`.
3. Extract the downloaded artifact archive. GitHub wraps uploaded artifacts in its own archive.

The Windows artifact contains Ini++ and the separately authored AssArraySC2 Runtime/Edittime `.mfx` pairs in separate directories. The new project builds from `extensions/AssArraySC2/` and the initialized `lSDK` submodule; it does not build, rename or require the original prebuilt `AssArray.mfx`/`AssArrayruntime.mfx` binaries.

The Android artifact contains the NDK `.so` files, `INI++.zip`, and distinct `ASSARRAYSC2.zip` with the five-ACE Java runtime. Its standalone store test uses the flat `ringdata.sav` fixture from Sonic Classic 1, not SC2 save evidence. Install the overlays at `<Fusion>/Data/Runtime/Android/INI++.zip` and `<Fusion>/Data/Runtime/Android/ASSARRAYSC2.zip`. The ZIPs contain no `.class`, `.jar`, `.aar`, AndroidX imports or proprietary exporter files. They remain unverified as Fusion 295.10 extensions until exporter merge/registration, target MFA export and device tests pass.

## 4. Install the Windows MFX build

Close Fusion first and back up any existing files. From the extracted `windows-mfx` artifact, copy:

- `runtime/INI++.mfx` to `<Fusion 2.5>/Data/Runtime/Unicode/INI++.mfx`
- `edittime/INI++.mfx` to `<Fusion 2.5>/Extensions/Unicode/INI++.mfx`
- `runtime/AssArraySC2.mfx` to `<Fusion 2.5>/Data/Runtime/Unicode/AssArraySC2.mfx`
- `edittime/AssArraySC2.mfx` to `<Fusion 2.5>/Extensions/Unicode/AssArraySC2.mfx`

Restart Fusion and open a **copy** of the Sonic MFA. Use Fusion's Replace Object workflow to select **AssArray SC2 Compatible** for the original AssArray object. Verify the four targeted actions and `GetValue` retain their IDs and parameters without manual event conversion. This replacement is intended to preserve those events; CI cannot verify the editor operation or target MFA behavior. The new MFX has its own internal identity and does not replace the original `ASSARRAY` installation. For manual MSBuild commands, see [BUILD.md](BUILD.md).

## 5. Install and validate Android packages

The Ini++ Java adapter uses the Fusion 293.0 `CRunExtension` API surface as a source baseline. Its ZIP follows the Clickteam runtime-merge layout and leaves Gradle, AndroidX, target SDK and JDK settings to the Fusion 295.10 exporter. To build/package the Ini++ overlay locally, see [BUILD.md](BUILD.md) and use `tools/package_android_extension.sh` after the NDK build.

For the separate AssArraySC2 Android runtime, package and integrate the distinct object with:

```sh
tools/package_assarray_sc2_android_extension.sh --out build/android-package
tools/integrate_assarray_sc2_android_exporter.sh \
  --exporter /path/to/extracted-Fusion-295.10-project
```

The package is `ASSARRAYSC2.zip`. The helper checks the known Fusion Java API surface and registers `CRunAssArraySC2` for `ASSARRAYSC2` in `CExtLoad.java` without replacing the vendor loader. These are source/API checks, not a full Fusion Java/Gradle compile. Export the target MFA with Fusion 2.5 build 295.10, install the APK on a device and test the existing events before claiming support.

## 6. Local checks and remaining acceptance work

These checks run without the proprietary exporter or Android NDK:

```sh
python3 tools/gen_android_aces.py --check
python3 tools/check_assarray_sc2_extension.py
bash tools/run_host_tests.sh
SANITIZE=1 bash tools/run_host_tests.sh
bash tools/sync_workflow.sh --check
```

They verify metadata, host C++ behavior and ZIP layout only. A real Android build needs NDK `26.1.10909125`; Java/Gradle integration needs Fusion 295.10 and its supported toolchain. The checked-in `ringdata.sav` is a Sonic Classic 1 numeric fixture, and `SC2sf.ini` remains opaque. To generate a target sample, run Sonic Classic 2 and trigger the existing Save INI event at the exact path configured in the target MFA; retrieve the actual file from app-private storage or a desktop Fusion run, then test load/save/GetValue against it. No SC2 save compatibility is claimed until that test passes.

The configured workflow cannot access the licensed Fusion 295.10 exporter source or Sonic MFA and cannot perform physical-device testing. It also cannot prove that Replace Object preserves the target MFA's actions. See [BUILD.md](BUILD.md), [INSTALL.md](INSTALL.md), and [ASSARRAY_SC2.md](ASSARRAY_SC2.md) for additional scope and test boundaries.
