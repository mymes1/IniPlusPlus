# Build and installation guide using GitHub Actions

## Important status: Android is not a verified install yet

The workflow can build the project's shared-C++ host tests, Windows MFX files, and (on an Ubuntu runner with the pinned NDK) Android JNI libraries. It **does not** compile the Java adapter against the selected Fusion 2.5 build 295.10 exporter, export the Sonic MFA, or produce a device-tested APK. The Android artifact is an **unverified source/assets overlay**, not a standalone Fusion Android extension installer.

The repository's `AndroidSDK_Gradle.zip` identifies as Fusion 292.0 and must not be substituted for the selected 295.10 exporter. Object references and Ini++ custom-parameter payloads also remain unimplemented in the adapter; see [COMPATIBILITY.md](COMPATIBILITY.md). Do not treat a green workflow run as proof that Android support is complete.

## 1. Make the workflow available on GitHub

The workflow is `.github/workflows/main.yml`; its canonical mirror is `ci/workflows/build.yml`. The checked-in workflow runs on pushes, pull requests, and manual `workflow_dispatch` runs.

- Push the branch containing the workflow to GitHub. A GitHub App/token used to push changes to `.github/workflows/` must be authorized to update workflows. This is a permission for the GitHub connection, not a secret to put in the repository.
- Do not add `AndroidSDK_Gradle.zip`, the licensed exporter, or the Sonic MFA to GitHub Actions artifacts or source control.
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
| `android-jni` | Android NDK r26.1.10909125 build for `arm64-v8a`, `armeabi-v7a`, and `x86_64`; packages project-owned Java/native overlay | `android-jni-overlay-unverified` |
| `windows-mfx` | Windows Runtime and Edittime MFX builds using MSVC v143/Win32; checks runtime exports | `windows-mfx` |
| `status` | Adds an artifact/acceptance boundary summary to the run | None |

The Android job does **not** invoke Java/Gradle or Fusion. It has no authorized build-295.10 exporter source or MFA. A green Android job means only that the project-owned native library and source overlay were produced by CI.

## 3. Download artifacts

1. Open the successful workflow run in **Actions**.
2. At the bottom of the run summary, download `windows-mfx` or `android-jni-overlay-unverified`.
3. Extract the downloaded artifact archive. GitHub wraps each uploaded artifact in its own download archive.

The Windows artifact contains the Runtime and Edittime `.mfx` files in separate directories. It is a build output, not an installer executable; see the Windows instructions below.

The Android artifact contains the NDK `.so` files and an `INI++.zip` overlay. **That inner ZIP is not an installable/verified extension package.** It is intended for integration with the exact authorized Fusion 295.10 Gradle exporter and includes no proprietary exporter files.

## 4. Install the Windows MFX build

Close Fusion first and keep a backup of the current files. From the extracted `windows-mfx` artifact, copy:

- `runtime/INI++.mfx` to `<Fusion 2.5>/Data/Runtime/Unicode/INI++.mfx`
- `edittime/INI++.mfx` to `<Fusion 2.5>/Extensions/Unicode/INI++.mfx`

Restart Fusion and open a copy of the MFA. The CI build checks the MFX exports, but you must still verify the editor/runtime behavior in your Fusion installation. Do not overwrite an existing installation without a backup. For manual MSBuild commands, see [BUILD.md](BUILD.md).

## 5. Integrate Android with the exact Fusion 295.10 exporter

This is a developer integration workflow, **not a one-click installation**. It requires an authorized, extracted Fusion 2.5 build 295.10 Gradle exporter project and its supported JDK/Gradle/Android SDK versions. Those Java/Gradle versions must come from that exporter; do not copy the versions from the incompatible 292.0 archive.

The most reproducible route is to build native libraries locally with the pinned NDK:

```sh
export ANDROID_NDK_HOME=/path/to/android-ndk-r26.1.10909125
python3 tools/gen_android_aces.py --check
tools/build_android.sh --out build/android
```

Then validate and integrate against the authorized exporter project:

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

The helper rejects other runtime versions, copies the Java adapter and ABI libraries, and registers the extension in that exporter's `CExtLoad.java`. If using the CI Android artifact instead of a local NDK build, extract it and point `--android-dir` at the directory containing `assets/mmf/<ABI>/libIniPlusPlusBridge.so`; inspect the extracted paths before running the helper.

After the Gradle build, export the unchanged Sonic MFA from Fusion using the same 295.10 exporter, install the APK on a device, and test the ACEs and sandboxed file operations used by the game. Gradle compilation alone is not acceptance. The workflow cannot perform this proprietary Fusion export or device test.

## 6. Local checks and known build blockers

The following commands run without the proprietary exporter or Android NDK:

```sh
python3 tools/gen_android_aces.py --check
bash tools/run_host_tests.sh
SANITIZE=1 bash tools/run_host_tests.sh
bash tools/sync_workflow.sh --check
```

They verify source metadata and host C++ behavior only. A real Android build additionally needs NDK `26.1.10909125`; a Java/Gradle integration needs the authorized 295.10 exporter and its compatible toolchain. A completed release still requires Sonic MFA export and device testing. See [BUILD.md](BUILD.md), [INSTALL.md](INSTALL.md), and [COMPATIBILITY.md](COMPATIBILITY.md) for the full boundaries.
