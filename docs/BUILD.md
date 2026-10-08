# Building Ini++

Ini++ is one extension with two builds:

| Target | Output | Toolchain |
| --- | --- | --- |
| Windows (Fusion 2.5 / MMF2 Unicode 32-bit) | `INI++.mfx` | Visual Studio 2022, MSVC v143, **Win32** |
| Android (Fusion 2.5 Android exporter) | `CRunINI++.so` per ABI, packaged as `INI++.zip` | Android NDK r23+ (clang, C++20) + the Clickteam Android extension SDK |

Everything except the MFX build is testable on Linux/macOS as well; the Android sources are compiled with the host compiler for the tests in `tools/run_host_tests.sh`.

---

## 1. Source layout

| Path | What it is |
| --- | --- |
| `Runtime.cpp`, `ACEs.cpp`, `RunData.hpp`, `Settings.hpp`, `CustomParams.*`, `Menus.cpp` | The implementation. `Runtime.cpp`/`ACEs.cpp`/`RunData.hpp` are shared by the Windows and Android builds and only contain platform code behind `FUSION_ANDROID_RUNTIME` guards. |
| `Edittime.cpp`, `General.cpp`, `Properties.cpp`, `Extension.rc`, `Icon.bmp` | Edittime (editor) side. Not compiled for Android: the editor reads the properties out of the MFA on Windows and passes the serialized blob to the platform at run time. |
| `INI++15.vcxproj`, `Common.props`, `exports-*.def` | Windows build (configurations `Runtime`, `Edittime`, `Debug`; all Win32). |
| `lSDK/` | Submodule: the high-level SDK Ini++ is written against (public domain). Provides `FusionAPI.hpp`, the import libraries, and `lSDK/src/UnicodeUtilities.cpp`. |
| `android/fusion/` | The Windows-API shim that lets the shared sources compile unchanged off Windows: `FusionAPI.hpp` (types, `mv*`/`CNC_*`/`callRunTimeFunction` equivalents), `ParamFrame.hpp` (the ACE parameter frame), `Shlobj.h`/`Windows.h`, and a small `lSDK/`. |
| `android/runtime/` | Android platform layer (`AndroidRuntime.cpp`: app data directory, file I/O, logging, edit data deserialization) and the generated ACE tables (`AceDispatch.inc`, `AceDeclarations.inc`). |
| `android/jni/` | The extension itself (`IniPlusPlusExtension.cc`) plus `Android.mk`/`Application.mk` for `ndk-build`. |
| `android/sdk-shim/` | A *documented reconstruction* of the proprietary `RuntimeNative.h` (for reading the code, never for building a distributable extension). |
| `android/tests/` | Host test harness and the mock SDK headers used by it. |
| `tools/` | `gen_android_aces.py` (generates the ACE tables from `Menus.cpp`), `run_host_tests.sh`, `smoke_build_so.sh`, `build_android.sh`, `make_sdk_b64.sh` / `make_sdk_b64.ps1` (turn the Clickteam SDK header into the CI secret), `package_release.sh`, `sync_workflow.sh`. |

The Android build compiles **four** translations units: `Runtime.cpp`, `ACEs.cpp`, `android/runtime/AndroidRuntime.cpp` and `android/jni/IniPlusPlusExtension.cc` (plus `android/fusion/lSDK/UnicodeUtilities.cpp`).

---

## 2. The one proprietary dependency: the Clickteam Android extension SDK

The Android runtime calls extensions through a C structure called `RuntimeFunctions`, declared in the
SDK header `<RuntimeNative.h>`.  That header is part of the Clickteam Android extension SDK, which is
licensed and **cannot be redistributed** with this repository - the repository therefore ships no
copy of it:

| File | What it is | May it be used for a release build? |
| --- | --- | --- |
| `android/tests/mock-sdk/RuntimeNative.h` | host-test stand-in used by `tools/run_host_tests.sh` | **no** (the build scripts reject it) |
| `android/sdk-shim/RuntimeNative.h` | documented reconstruction, for reading the code | **no** (the build scripts reject it) |
| your extracted Clickteam SDK | the real header | yes |

Without the real header the Android build **fails with an explanation** instead of silently using a
stand-in.

### 2.1 Where to get it

1. Open <https://www.clickteam.com/extensions-sdks> and switch to the **Android SDK** tab.
2. Follow its **\[Download Android SDK\]** link - it points at the *Official Android SDK Release*
   thread on <http://community.clickteam.com/threads/89105-Official-Android-SDK-Release>.  The forum
   needs a **free Clickteam account** (log in or register, then download the attachment).
3. Extract the archive anywhere.  The relevant part is the header, which sits at
   `<sdk>/android/jni/RuntimeNative.h` in the official layout (the SDK's own
   `native/base/NativeExtension.h` includes `../../android/jni/RuntimeNative.h`); the archive also
   contains the `CRunTemplate` NDK project, `tools/install-native` and the `runtime/` directory that
   Clickteam's docs tell you to fill from `<Fusion>/Data/Runtime/Android/RuntimeAndroid.zip`.

Worth checking before you download: some Android exporter installations already carry a copy of the
SDK next to Fusion.  Search your Fusion folder for `RuntimeNative.h`
(`dir /s /b RuntimeNative.h` in `C:\Program Files (x86)\Clickteam Fusion 2.5`, or drag the folder
into Explorer's search box); if a copy shows up it is the same header and can be used as is.

The header is only needed to *compile*.  It is never shipped: `INI++.zip` contains only the compiled
`CRunINI++.so` files, and nothing in this repository redistributes the SDK.  Do not commit the header,
publish it in a gist, or add it to a release archive.

### 2.2 Using it locally (no CI)

```sh
tools/build_android.sh --sdk-dir /path/to/extracted/sdk
# or
export IPPP_ANDROID_SDK_DIR=/path/to/extracted/sdk
```

`tools/build_android.sh` looks in the directory itself and in the layouts seen in the wild
(`inc/`, `include/`, `jni/`, `android/jni/`, `android/inc/`, `native/include/`, `sdk/inc/`, ...) and
as a last resort searches up to six levels below `--sdk-dir`, so pointing it at the SDK root is
enough.  It rejects both stand-ins if they are pointed at by accident.

### 2.3 Using it in CI (the `CLICKTEAM_ANDROID_SDK_B64` secret)

The workflow is `.github/workflows/main.yml`, generated from `ci/workflows/build.yml` by
`tools/sync_workflow.sh` (the split exists because pushing a file under `.github/workflows/` needs a
token with the "workflows" permission; see that file's header).  Its `android-extension` job takes the
header from one of:

* the repository secret `CLICKTEAM_ANDROID_SDK_B64` - the base64 of a small `.zip` containing
  `RuntimeNative.h`, or
* the repository variable `IPPP_ANDROID_SDK_DIR` (a path that exists on a self-hosted runner).

If neither is configured the job fails and prints how to provide the SDK.  That is intentional: the
alternative would be a silently broken or non-redistributable Android extension.

#### Making the base64 string

```sh
# Linux / macOS / WSL / Git Bash - the helper checks the header, packs it and writes the .b64 file
tools/make_sdk_b64.sh --sdk-dir /path/to/extracted/sdk --out clickteam-android-sdk.b64
tools/make_sdk_b64.sh --zip /path/to/clickteam-android-sdk.zip --out clickteam-android-sdk.b64
```

```powershell
# Windows (PowerShell) - same helper, Windows version
powershell -ExecutionPolicy Bypass -File tools\make_sdk_b64.ps1 -SdkDir C:\sdk\clickteam-android
```

By hand, if you prefer:

```sh
# Linux (GNU coreutils)
cd /path/to/extracted/sdk && zip -r ../clickteam-android-sdk.zip android/jni
base64 -w0 ../clickteam-android-sdk.zip > clickteam-android-sdk.b64

# macOS (BSD base64: -b 0 = no line breaks)
cd /path/to/extracted/sdk && zip -r ../clickteam-android-sdk.zip android/jni
base64 -b 0 ../clickteam-android-sdk.zip > clickteam-android-sdk.b64

# any OS with python3 instead of a zip tool
python3 -c "import base64,pathlib;d=pathlib.Path('android/jni');z=pathlib.Path('clickteam-android-sdk.zip');import zipfile;f=zipfile.ZipFile(z,'w',zipfile.ZIP_DEFLATED);[f.write(p,p.as_posix()) for p in d.rglob('*.h')];f.close();pathlib.Path('clickteam-android-sdk.b64').write_text(base64.b64encode(z.read_bytes()).decode())"
```

```powershell
# Windows without the helper: zip the header's folder, then base64 it into a file
Compress-Archive -Path C:\sdk\clickteam-android\android -DestinationPath .\clickteam-android-sdk.zip -Force
[Convert]::ToBase64String([IO.File]::ReadAllBytes('.\clickteam-android-sdk.zip')) |
  Set-Content -NoNewline .\clickteam-android-sdk.b64
```

The zip structure is not checked in any way that matters - the workflow extracts it and searches for
`RuntimeNative.h`.  All of these work:

```
clickteam-android-sdk.zip
├── RuntimeNative.h                     <- header alone at the root (smallest)
├── inc/RuntimeNative.h                 <- header in a subfolder
└── android/jni/RuntimeNative.h         <- as shipped by Clickteam (plus the headers next to it,
                                            in case RuntimeNative.h includes them)
```

Keep the archive small: it holds **headers only, never the whole SDK** with its binaries, templates
and runtime sources.  GitHub rejects repository secrets larger than 48 kB, so a base64 string much
above ~48,000 characters will not save.  If it does not fit, put just `RuntimeNative.h` in the zip;
if the compiler then complains about a missing include, add that header too.

#### Setting the secret

```sh
gh secret set CLICKTEAM_ANDROID_SDK_B64 --repo mymes1/IniPlusPlus < clickteam-android-sdk.b64
```

or in the browser: *Settings -> Secrets and variables -> Actions -> New repository secret*, name
`CLICKTEAM_ANDROID_SDK_B64`, paste the file's contents (it is a single long line).

For a self-hosted runner, *Settings -> Secrets and variables -> Actions -> **Variables** -> New
repository variable*, name `IPPP_ANDROID_SDK_DIR`, value e.g. `C:\sdk\clickteam-android`.

#### Verifying it worked

Re-run the *Build* workflow.  The `android-extension` job's first step prints the header it found:

```
Extracted the Clickteam Android SDK header archive:
/opt/hostedtoolcache/.../clickteam-android-sdk/android/jni/RuntimeNative.h
```

and `tools/build_android.sh` then prints `Android SDK include directory: ...` before invoking
`ndk-build`.  If you still see `::error title=Clickteam Android SDK required`, the secret/var was not
visible to the job (wrong repository, wrong name, or the workflow file on the default branch does not
contain the step yet).

### CI - what the Android job does with it

The `android-extension` job checks out the repository, installs the NDK version from section 3,
materialises the header from the secret/variable, runs `tools/build_android.sh` for the three ABIs and
packages `dist/android-package/` (`INI++.zip` + `assets/mmf/<abi>/CRunINI++.so`) for the `package`
job.

---

## 3. Versions used by the CI build

| Component | Version | Notes |
| --- | --- | --- |
| MSVC / Visual Studio | VS 2022, MSVC v143, Windows SDK 10 | `PlatformToolset v143`; the project is Win32 only (Fusion 2.5 is 32-bit). |
| Android NDK | r26 (`26.1.10909125`), r23 or newer required | C++20 (`std::filesystem`, `std::to_chars`, `std::bit_cast`), clang, `ndk-build`. |
| `APP_PLATFORM` | `android-21` | Minimum the Fusion Android runtime supports. |
| `APP_STL` | `c++_static` | The Fusion-generated APK does not ship `libc++_shared.so`. |
| Python | 3.9+ | Only for `tools/gen_android_aces.py`. |
| Host compiler for the tests | GCC 11+/Clang 14+ | C++20. |
| **For building APKs with the Android exporter** (not needed to build this extension, but needed to use it) | JDK **11** (17+ breaks the exporter), Android SDK Platform **API 34**, Build-Tools **34.x**, Gradle (bundled with the exporter), matching NDK | See the exporter's own setup guide; the extension itself only needs `ndk-build`. |

Nothing in the build silently substitutes a different SDK: if the toolchain is missing or too old, the build fails with a message naming the component.

---

## 4. Windows: `INI++.mfx`

From a "x64 Native Tools Command Prompt for VS 2022" (the MSBuild host architecture does not matter, the target is always Win32):

```bat
git submodule update --init --recursive

:: the project's platform is Win32 (.sln spells it x86)
msbuild INI++15.vcxproj /m /p:Configuration=Runtime  /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild INI++15.vcxproj /m /p:Configuration=Edittime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false

copy Build\Output\Runtime\INI++15.mfx   INI++.mfx
copy Build\Output\Edittime\INI++15.mfx  INI++_edittime.mfx
```

* `Runtime|Win32` builds the **runtime** MFX (`FUSION_RUNTIME_ONLY`, `exports-runtime.def`): the file that is installed as `Data\Runtime\Unicode\INI++.mfx` and that ends up inside built applications and players.
* `Edittime|Win32` builds the **edittime** MFX (`exports-edittime.def`): the file Fusion's editor loads from `Extensions\Unicode\`.
* `PostBuildEventUseInBuild=false` disables lSDK's `AutoInstall.bat`, which tries to copy the MFX into a local Fusion installation.
* The project's target name is `INI++15`; both outputs are installed as `INI++.mfx` (Fusion matches the edittime and runtime files by name, existing MFAs reference `INI++.mfx`, and the Android build derives the extension name "INI++" from that file name).

The build needs no Clickteam SDK files: lSDK (public domain) provides the headers and import libraries.

---

## 5. Android: `CRunINI++.so`

```sh
# once, if the NDK is not on PATH:
export ANDROID_NDK_HOME=/path/to/android-ndk-r26

# the Clickteam Android SDK header, see section 2:
export IPPP_ANDROID_SDK_DIR=/path/to/extracted/clickteam-android-sdk

# regenerate the ACE tables from Menus.cpp/ACEs.cpp (also run automatically)
python3 tools/gen_android_aces.py

# build every ABI in android/jni/Application.mk into build/android/libs/<abi>/
# and copy + verify the packaged .so files into build/android/<abi>/
tools/build_android.sh --out build/android
```

Output (one pair per ABI: the name the runtime looks for, plus the plain-identifier spelling):

```
build/android/arm64-v8a/CRunINI++.so
build/android/arm64-v8a/CRunIniPlusPlus.so
build/android/armeabi-v7a/...
build/android/x86_64/...
```

To build a single ABI or to use a different extension name:

```sh
tools/build_android.sh --abis arm64-v8a
# MFX renamed to something that is a valid C identifier, e.g. IniPlusPlus.mfx:
tools/build_android.sh --sdk-dir ... --name=CRunIniPlusPlus   # nothing to change, that is the default
```

The build fails if a `CRunINI++_*` entry point is missing from the produced `.so` (`nm -D` check), which is how a wrong compiler flag or a missing ACE table is caught early.

### 5.1 What the build produces, and where it has to end up

The Fusion 2.5 Android exporter merges ZIP archives from `<Fusion>/Data/Runtime/Android/` into the runtime project before building the APK, and the Android runtime resolves native extensions from `assets/mmf/<abi>/CRun<Name>.so`, where `<Name>` is derived from the MFX file name (`INI++.mfx` → `INI++`). The CI packages exactly that:

```
INI++.zip
└── assets/mmf/<abi>/CRunINI++.so      (arm64-v8a, armeabi-v7a, x86_64)
```

`docs/INSTALL.md` describes installing it and what to check in the resulting APK.

### 5.2 The installable release archive

`tools/package_release.sh` assembles everything a Fusion 2.5 user needs into one archive - the
Windows MFX pair, the Android natives with their Fusion package, an installer script, the
documentation and (with `--source`) the sources:

```sh
tools/package_release.sh \
    --windows-runtime build/windows/INI++.mfx \
    --windows-edittime build/windows/INI++_edittime.mfx \
    --android-dir build/android \
    --out build/release --source
```

The result is `build/release/IniPlusPlus-<version>.zip`:

```
IniPlusPlus-<version>/
├── README.txt                        what is inside, quick install for both platforms
├── windows/
│   ├── install.bat                   detects Fusion 2.5 / MMF2 installs and copies both MFXs
│   ├── edittime/INI++.mfx            -> <Fusion>\Extensions\Unicode\INI++.mfx
│   └── runtime/INI++.mfx             -> <Fusion>\Data\Runtime\Unicode\INI++.mfx
├── android/
│   ├── INI++.zip                     -> <Fusion>\Data\Runtime\Android\INI++.zip
│   └── assets/mmf/<abi>/CRunINI++.so the same natives, unpacked
├── docs/{INSTALL,COMPATIBILITY,BUILD}.md
└── source/IniPlusPlus-<version>-source.tar.gz     (with --source)
```

`--android-dir` accepts either the `--out` directory of `tools/build_android.sh`
(`<abi>/CRunINI++.so`) or a package folder (`assets/mmf/<abi>/CRunINI++.so`, optionally with a
ready-made `INI++.zip`).

Without the binaries the script stops and lists what is missing; `--allow-missing` only exists for
layout tests and stamps `MISSING-BINARIES.txt` into the archive. The CI workflow's `package` job
runs the same script with the MFX from the Windows job and the Android package from the NDK job, and
uploads the archive as the `release-bundle` artifact - that is the file to hand to users.

The CI workflow also uploads a Windows-only archive (`IniPlusPlus-<version>-windows.zip`, artifact
`windows-mfx`) with the same `windows/` layout, for users who only need the MFX and do not have the
Clickteam Android SDK, and the Android package on its own (`INI++.zip`, artifact
`android-extension`).

---

## 6. Host tests (no NDK, no Clickteam SDK, no Fusion)

```sh
bash tools/run_host_tests.sh                 # behaviour tests through the exported entry points
SANITIZE=1 bash tools/run_host_tests.sh      # the same, with ASan/UBSan
bash tools/smoke_build_so.sh                 # builds the .so, checks nm -D and dlopen/dlsym
tools/build_android.sh --check-only          # ACE tables match Menus.cpp/ACEs.cpp (no SDK, no NDK)
tools/sync_workflow.sh --check                # .github/workflows/build.yml matches ci/workflows/build.yml
python3 tools/gen_android_aces.py --report   # which ACEs are implemented, which are stubs
```

`run_host_tests.sh` compiles `Runtime.cpp`, `ACEs.cpp`, `android/runtime/AndroidRuntime.cpp` and `android/jni/IniPlusPlusExtension.cc` with `-DFUSION_ANDROID_RUNTIME` and drives the *exported entry points* through a mock of the Fusion Android runtime: it creates an object from a byte-identical copy of the edit data the Windows editor writes, runs the same ACE numbers Fusion would, and checks groups, items, numbers, strings, save/load/autosave and the current-group behaviour.

These tests prove the logic, the ACE dispatch and the file layer; they cannot prove the Clickteam ABI, which only exists in the proprietary runtime (see the verification matrix in `docs/COMPATIBILITY.md`).

---

## 7. Troubleshooting

| Symptom | Cause / fix |
| --- | --- |
| `error: ndk-build was not found` | Install the NDK (r23+) and put `ndk-build` on `PATH`, or pass `--ndk`/`ANDROID_NDK_HOME`. |
| `error: RuntimeNative.h was not found under ...` | The Clickteam Android extension SDK was not extracted there; see section 2. |
| `RuntimeNative.h was not found` / `#error` while compiling `IniPlusPlusExtension.cc` | Same as above. `android/tests/mock-sdk/RuntimeNative.h` is only for the host tests and `tools/smoke_build_so.sh`. |
| CI: "Clickteam Android SDK required" | The `android-extension` job needs the secret/variable from section 2; the `host-tests`, `ace-tables` and `windows-extension` jobs do not. |
| `build_android.sh`: "RuntimeNative.h was not found under ..." | `--sdk-dir` points at something that is not the extracted SDK (or the download was the wrong tab of the SDK page). The header is at `<sdk>/android/jni/RuntimeNative.h` in the official archive; the script searches that layout too, so pointing at the SDK root works. |
| `make_sdk_b64.sh`: "this is ... the host-test stand-in" | You pointed it at `android/tests/mock-sdk/` or `android/sdk-shim/` instead of the extracted Clickteam SDK. |
| Secret will not save: too large / "must be less than 48 KB" | Pack headers only - `tools/make_sdk_b64.sh` does that by default; if the header's own directory is still too big, zip `RuntimeNative.h` alone. |
| `package_release.sh` ends with "incomplete package - missing: ..." | The MFX/`.so` paths it was given do not exist. Build them first (sections 4 and 5), or use `--allow-missing` only to check the layout. |
| `does not export CRunINI++_...` | The toolchain dropped the assembler aliases (they are guarded by `IPPP_ANDROID_NO_MANGLED_SYMBOLS`); build with a recent NDK, or rename the MFX to a valid identifier and rebuild with `-DIPPP_ANDROID_EXT_NAME=...`. |
| App starts but the object is missing: `*** MISSING EXTENSION: INI++` | The `.so` is not in `assets/mmf/<abi>/` for the device's ABI (or the MFA's extension name differs from the file name). |
| The extension loads but every ACE does nothing | The ACE tables are stale: run `python3 tools/gen_android_aces.py` and rebuild. |
| Android build fails on `std::filesystem`/`std::to_chars` | NDK too old: C++20 support needs r23+. |
| Windows build fails to link `mmfs2u.lib` | Submodule missing: `git submodule update --init --recursive`. |
