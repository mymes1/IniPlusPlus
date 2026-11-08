# Build and installation guide using GitHub Actions

## Important status: Android is not a verified install yet

The workflow can build the project's shared-C++ host tests, Ini++ Windows MFX files, and (in the Ubuntu `android-jni` job) Android JNI libraries. That Android job also compiles/tests the standalone AssArray INI store against the Sonic Classic 1 `ringdata.sav` format fixture and packages the Java-only `ASSARRAY.zip`. It does **not** compile the full adapter with the Fusion 2.5 build 295.10 exporter, test a Sonic Classic 2 save, export the Sonic MFA, or produce a device-tested APK. The Android ZIPs remain **unverified in the target exporter**.

The repository's `AndroidSDK_Gradle.zip` identifies as Fusion 293.0 and is used only as a Java API reference. The package is designed for the Fusion 295.10 exporter to merge and compile source; its Gradle/SDK configuration and any AndroidX dependencies come from that exporter and are not bundled. Object references and Ini++ custom-parameter payloads also remain unimplemented; see [COMPATIBILITY.md](COMPATIBILITY.md). Do not treat a green workflow run as proof that Android support is complete.

## 1. Make the workflow available on GitHub

The workflow is `.github/workflows/main.yml`; its canonical mirror is `ci/workflows/build.yml`. The checked-in workflow runs on pushes, pull requests, and manual `workflow_dispatch` runs.

- Push the branch containing the workflow to GitHub. A GitHub App/token used to push changes to `.github/workflows/` must be authorized to update workflows. This is a permission for the GitHub connection, not a secret to put in the repository.
- Do not upload `AndroidSDK_Gradle.zip`, the licensed exporter, or the Sonic MFA to GitHub Actions artifacts, runtime extension ZIPs or release bundles. The supplied root archive is inspected only as an API reference.
- A push to a branch containing the workflow starts it automatically. For **Run workflow** (`workflow_dispatch`), GitHub requires the workflow file to exist on the repository's default branch; until it is merged there, use a push-triggered run on the branch or merge it first.

Before pushing workflow changes, verify the two YAML files match:

```sh
bash tools/sync_workflow.sh --check
```

## 2. Run and understand the jobs

A successful run has these jobs:

| Job | What it checks/builds | Artifact |
| --- | --- | --- |
| `host-tests` | Generated ACE metadata, shared C++ bridge and sandbox tests, ASan/UBSan tests, workflow mirror | None |
| `android-jni` | Ubuntu Android NDK r26.1.10909125 build for `arm64-v8a`, `armeabi-v7a`, and `x86_64`; tests the AssArray store against the Sonic Classic 1 format fixture; packages Ini++ and AssArray Android ZIPs | `android-jni-overlay-unverified` |
| `windows-mfx` | Builds the Ini++ Runtime and Edittime MFX files using MSVC v143/Win32; checks runtime exports | `windows-mfx` |
| `status` | Adds an artifact/acceptance boundary summary to the run | None |

The Android job uses JDK 17 to compile/test the standalone AssArray INI store, but it does **not** compile the full Java adapter against Fusion classes or run the Fusion Gradle exporter. It has no Fusion 295.10 exporter project or MFA. A green job does not validate ZIP ingestion, `CExtLoad` registration, unchanged MFA export or device compatibility.

## 3. Download artifacts

1. Open the successful workflow run in **Actions**.
2. At the bottom of the run summary, download `windows-mfx` or `android-jni-overlay-unverified`.
3. Extract the downloaded artifact archive. GitHub wraps each uploaded artifact in its own download archive.

The Windows artifact contains the Ini++ Runtime and Edittime `.mfx` files in separate directories. It does not build AssArray: this checkout has only the prebuilt `AssArray.mfx` and `AssArrayruntime.mfx` binaries, with no AssArray source project. A real AssArray MFX compile job needs the original source and build dependencies.

The Android artifact contains the NDK `.so` files, an `INI++.zip`, and an `ASSARRAY.zip` with the five-ACE Java-only AssArray adapter. The AssArray store is tested against the flat `ringdata.sav` format fixture from Sonic Classic 1; this is not a Sonic Classic 2 compatibility test, and the full adapter is not compiled against Fusion classes. Install the ZIPs at `<Fusion>/Data/Runtime/Android/INI++.zip` and `<Fusion>/Data/Runtime/Android/ASSARRAY.zip`. Neither ZIP contains `.class`, `.jar`, `.aar`, AndroidX imports, or proprietary exporter files. **Neither is verified as a Fusion 295.10 extension:** exporter merge/registration, unchanged MFA export and device tests remain acceptance steps.

## 4. Install the Windows MFX build

Close Fusion first and keep a backup of the current files. From the extracted `windows-mfx` artifact, copy:

- `runtime/INI++.mfx` to `<Fusion 2.5>/Data/Runtime/Unicode/INI++.mfx`
- `edittime/INI++.mfx` to `<Fusion 2.5>/Extensions/Unicode/INI++.mfx`

Restart Fusion and open a copy of the MFA. The CI build checks the MFX exports, but you must still verify the editor/runtime behavior in your Fusion installation. Do not overwrite an existing installation without a backup. For manual MSBuild commands, see [BUILD.md](BUILD.md).

## 5. Install and validate the source ZIP with Fusion 295.10

The Java adapter uses the Fusion 293.0 `CRunExtension` API surface as its source baseline. The generated archive follows the Clickteam runtime-merge layout and leaves Gradle, AndroidX, target SDK and JDK settings to the target Fusion exporter.

If building locally, produce and install the ZIP with:

```sh
export ANDROID_NDK_HOME=/path/to/android-ndk-r26.1.10909125
python3 tools/gen_android_aces.py --check
tools/build_android.sh --out build/android
tools/package_android_extension.sh \
  --android-dir build/android \
  --out build/android-package \
  --install-to "/path/to/Fusion/Data/Runtime/Android"
```

Alternatively, download the CI artifact, extract it, and copy its `INI++.zip` into `<Fusion>/Data/Runtime/Android/`. The archive should contain raw sources at `src/Extensions/` and native libraries at `assets/mmf/<ABI>/`; it contains no old JAR/AAR or proprietary exporter ZIP.

Export an unchanged MFA with Fusion 2.5 build 295.10. Confirm that the target exporter merges and compiles the Java source and registers `CRunIniPlusPlus` in `CExtLoad`. If the installed extension ZIP is not registered automatically, the generated `CExtLoad-registration.patch` is a manual reference. A direct extracted-project check is also available:

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

That helper checks API members and can compile with the target project's own Gradle wrapper; it does not prove Fusion's extension ZIP installation path. After export, install the APK on a device and test the ACEs and sandboxed file operations used by the game. Compilation alone is not acceptance. The workflow cannot perform a proprietary Fusion export or device test. For the AssArray Java-only subset, use `tools/package_assarray_android_extension.sh --out build/android-package` and `tools/integrate_assarray_android_exporter.sh --exporter /path/to/extracted-exporter` to package/register the five ACEs. This does not build AssArray Windows MFX files.

## 6. Local checks and known build blockers

The following commands run without the proprietary exporter or Android NDK:

```sh
python3 tools/gen_android_aces.py --check
bash tools/run_host_tests.sh
SANITIZE=1 bash tools/run_host_tests.sh
bash tools/sync_workflow.sh --check
```

They verify source metadata, host C++ behavior and the ZIP's merge layout only. A real Android build additionally needs NDK `26.1.10909125`; Java/Gradle integration needs Fusion 295.10 and its compatible toolchain. A completed release still requires `CExtLoad` registration, Sonic MFA export and device testing. See [BUILD.md](BUILD.md), [INSTALL.md](INSTALL.md), and [COMPATIBILITY.md](COMPATIBILITY.md) for the full boundaries.
