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
| `tools/` | `gen_android_aces.py` (generates the ACE tables from `Menus.cpp`), `run_host_tests.sh`, `smoke_build_so.sh`, `build_android.sh`. |

The Android build compiles **four** translations units: `Runtime.cpp`, `ACEs.cpp`, `android/runtime/AndroidRuntime.cpp` and `android/jni/IniPlusPlusExtension.cc` (plus `android/fusion/lSDK/UnicodeUtilities.cpp`).

---

## 2. The one proprietary dependency: the Clickteam Android extension SDK

The Android runtime calls extensions through a C structure called `RuntimeFunctions`, declared in the SDK header `<RuntimeNative.h>`. That header is part of the Clickteam Android extension SDK, which is licensed and **cannot be redistributed** with this repository (the repository therefore ships no copy of it - `android/sdk-shim/RuntimeNative.h` is a reconstruction with a big "do not ship" sign on it, and `android/tests/mock-sdk/RuntimeNative.h` is a host-test stand-in).

Get the SDK from <https://www.clickteam.com/extensions-sdks> ("Android" section, the same SDK the Android exporter uses; the exporter also installs a copy of the SDK next to Fusion). Then either

```sh
tools/build_android.sh --sdk-dir /path/to/extracted/sdk
# or
export IPPP_ANDROID_SDK_DIR=/path/to/extracted/sdk
```

`tools/build_android.sh` probes the usual layouts below `--sdk-dir` (`inc/`, `Inc/`, `include/`, `android/inc/`, ...) and picks the directory that actually contains `RuntimeNative.h`, so pointing it at the SDK root is enough.

If you already have the Android exporter installed, the header is inside `<Fusion>/Data/Runtime/Android/RuntimeAndroid.zip` (extract it) or next to the exporter's NDK projects; any extracted copy works.

Without the header the Android build **fails with an explanation** instead of falling back to the stand-in - see the error text in `tools/build_android.sh`.

### CI

The CI workflow (kept at `ci/workflows/build.yml`, see that file's header for why and how to install it as `.github/workflows/build.yml`) takes the header, in its `android-extension` job, from one of:

* the repository secret `CLICKTEAM_ANDROID_SDK_B64` - a base64-encoded `.zip` whose root contains `RuntimeNative.h` (and any headers it includes):

  ```sh
  cd /path/to/extracted/sdk && zip -r ../clickteam-android-sdk.zip .
  base64 -w0 ../clickteam-android-sdk.zip    # paste into the repository secret
  ```

* the repository variable `IPPP_ANDROID_SDK_DIR` (a path on a self-hosted runner).

If neither is configured the job fails and prints how to provide the SDK. That is intentional: the alternative would be a silently broken or non-redistributable Android extension.

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
| `package_release.sh` ends with "incomplete package - missing: ..." | The MFX/`.so` paths it was given do not exist. Build them first (sections 4 and 5), or use `--allow-missing` only to check the layout. |
| `does not export CRunINI++_...` | The toolchain dropped the assembler aliases (they are guarded by `IPPP_ANDROID_NO_MANGLED_SYMBOLS`); build with a recent NDK, or rename the MFX to a valid identifier and rebuild with `-DIPPP_ANDROID_EXT_NAME=...`. |
| App starts but the object is missing: `*** MISSING EXTENSION: INI++` | The `.so` is not in `assets/mmf/<abi>/` for the device's ABI (or the MFA's extension name differs from the file name). |
| The extension loads but every ACE does nothing | The ACE tables are stale: run `python3 tools/gen_android_aces.py` and rebuild. |
| Android build fails on `std::filesystem`/`std::to_chars` | NDK too old: C++20 support needs r23+. |
| Windows build fails to link `mmfs2u.lib` | Submodule missing: `git submodule update --init --recursive`. |
