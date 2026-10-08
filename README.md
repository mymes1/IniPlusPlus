# Ini++ (Unicode)

Ini++ is an INI-file extension object for Clickteam Fusion 2.5 / Multimedia Fusion 2 (Unicode builds). It stores group/item/value data in ordinary INI files, with an undo/redo stack, escaping, encryption, autosave and a large set of actions, conditions and expressions.

This tree is the `unicode` branch, extended with a **Fusion 2.5 Android runtime implementation**: the same extension, the same object, the same ACEs - additionally usable in apps built with the Android exporter, without changing a single event in an existing MFA.

* Language: C++20, platform-neutral core
* Windows target: Fusion 2.5 / MMF2 Unicode -> `INI++.mfx`
* Android target: Fusion 2.5 Android exporter -> `CRunINI++.so` per ABI, packaged as `INI++.zip`

## Documentation

| Document | Contents |
| --- | --- |
| [docs/BUILD.md](docs/BUILD.md) | Toolchain versions, the Clickteam Android SDK dependency, exact build commands (MSBuild / ndk-build), host tests, troubleshooting |
| [docs/INSTALL.md](docs/INSTALL.md) | Installing and using the extension in Fusion 2.5, on Windows and on Android |
| [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md) | How the Android build works, what is verified and how, all platform differences and limitations |
| [ci/workflows/build.yml](ci/workflows/build.yml) | The CI workflow. `.github/workflows/main.yml` is a byte-for-byte copy of it (`tools/sync_workflow.sh`, `--check` to verify) so that neither the branch nor a token without the "workflows" permission can get them out of sync. Jobs: ACE-table check, host tests (incl. sanitizers), Windows MFX + Windows-only zip, Android `.so` + `INI++.zip`, release packaging |

## Layout

| Path | Contents |
| --- | --- |
| `Runtime.cpp`, `ACEs.cpp`, `RunData.hpp`, `Settings.hpp`, `CustomParams.*`, `Menus.cpp` | The extension itself. Shared by the Windows and Android builds (`FUSION_ANDROID_RUNTIME` guards platform code). |
| `Edittime.cpp`, `General.cpp`, `Properties.cpp`, `Extension.rc`, `Icon.bmp` | Edittime side: object registration, properties, resources (Windows editor). |
| `lSDK/` | Submodule with the high-level SDK the extension is written against (public domain). |
| `android/fusion/`, `android/runtime/`, `android/jni/` | Android support: Windows-API shim, platform layer + generated ACE tables, and the extension's JNI/ABI entry points with `Android.mk`/`Application.mk`. |
| `tools/` | `gen_android_aces.py` (generates the ACE tables from `Menus.cpp`), `run_host_tests.sh`, `smoke_build_so.sh`, `build_android.sh`. |
| `docs/` | Build, install and compatibility documentation. |
| `packaging/` | `install-windows.bat` (installs both MFXs into a detected Fusion) and the README that goes into the release archive. |

## Quick start

```sh
git submodule update --init --recursive

# host tests - no Fusion, no NDK, no Clickteam SDK needed
bash tools/run_host_tests.sh
bash tools/smoke_build_so.sh

# Windows MFX (MSVC, from a VS developer prompt; the project platform is Win32)
msbuild INI++15.vcxproj /m /p:Configuration=Runtime  /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild INI++15.vcxproj /m /p:Configuration=Edittime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false

# Android .so (needs the NDK and the proprietary Clickteam Android SDK header, see docs/BUILD.md)
tools/build_android.sh --sdk-dir /path/to/clickteam-android-sdk --out build/android

# the installable release for Fusion 2.5 (Windows MFX + Android package + installer + docs)
tools/package_release.sh --windows-runtime <runtime.mfx> --windows-edittime <edittime.mfx> \
                         --android-dir build/android --out build/release --source
```

The CI workflow builds all of the above and uploads the same release archive as the
`release-bundle` artifact (`IniPlusPlus-<version>.zip`); its layout and the installer are described
in [docs/BUILD.md § 5.2](docs/BUILD.md) and [docs/INSTALL.md](docs/INSTALL.md).

## History

* Original Ini++ 1.x/2.x by Jax (Jamie McLaughlin): <https://github.com/LB--/Fusion-INI-plus-plus>
* Unicode rewrite and this fork: the `unicode` branch, maintained at <https://github.com/mymes1/IniPlusPlus>
