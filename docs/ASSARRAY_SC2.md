# AssArray SC2-compatible replacement object

## New object identity and scope

This project adds a **new, editor-visible Fusion extension object** named **AssArray SC2 Compatible**. Its extension identity is `ASSARRAYSC2` (runtime FourCC `AS2C`); it is separate from the original `ASSARRAY` object and is designed for Fusion's **Replace Object** workflow.

The editor exposes only the five operations used by the Sonic Classic 2 event set—not an SC1+SC2 union:

| ACE | ID | Fusion parameter signature | Runtime behavior |
| --- | ---: | --- | --- |
| Set Value at Key | Action 0 | `PARAM_EXPSTRING` key, `PARAM_EXPRESSION` value | Stores a finite numeric value. |
| Clear the Array | Action 3 | none | Removes all stored values. |
| Load INI File into Values | Action 12 | `PARAM_EXPSTRING` path | Loads numeric `key=value` assignments. |
| Save INI File from Values | Action 14 | `PARAM_EXPSTRING` path | Saves a flat numeric `key=value` file. |
| `GetValue` | Expression 0 | `EXPPARAM_STRING` key | Returns the numeric value, or zero if missing. |

These IDs and parameter types match the selected AssArray ACE metadata used by the SC2 target. The MFX reserves dispatch slots through Action 14 and keeps the original 12-condition table extent; these are table slots only. No other action, condition or expression is exposed, and condition stubs return false. No SC1-only ACEs are added.

Matching IDs/signatures are intended to preserve the relevant events during Replace Object. The actual operation still needs to be exercised on a copy of the target MFA; CI does not have that MFA and cannot verify conversion behavior.

## Independent Windows project

`extensions/AssArraySC2/AssArraySC2.sln` and `AssArraySC2.vcxproj` are an independently authored Win32 editor/runtime MFX project. It owns its ACE menus/metadata, runtime dispatch, resources, icon and numeric flat-INI store. It builds against the initialized `lSDK` submodule and does not include, link, modify or require original AssArray source or the root prebuilt `AssArray.mfx`/`AssArrayruntime.mfx` binaries. Those original files remain untouched. The project uses C++20 (`std::span` is used by the pinned lSDK) and MSVC permissive mode (`ConformanceMode=false`), matching the lSDK example project; the lSDK `FusionParams` helper relies on MSVC's permissive dependent-base member lookup.

From a Visual Studio 2022 developer prompt:

```bat
git submodule update --init --recursive
msbuild extensions/AssArraySC2/AssArraySC2.vcxproj /m /p:Configuration=Runtime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
msbuild extensions/AssArraySC2/AssArraySC2.vcxproj /m /p:Configuration=Edittime /p:Platform=Win32 /p:PostBuildEventUseInBuild=false
```

Outputs:

- `build/mfx/AssArraySC2/Runtime/AssArraySC2.mfx` → `<Fusion 2.5>/Data/Runtime/Unicode/AssArraySC2.mfx`
- `build/mfx/AssArraySC2/Edittime/AssArraySC2.mfx` → `<Fusion 2.5>/Extensions/Unicode/AssArraySC2.mfx`

The MFX advertises Fusion build 295 compatibility. Windows CI run [42492164267](https://github.com/mymes1/IniPlusPlus/actions/runs/42492164267) successfully built Runtime and Edittime and passed the required export checks. This validates compilation/artifact checks only; it is not a Fusion editor/runtime smoke test.

### Fusion Replace Object workflow

1. Back up the MFA and install the separate Edittime/Runtime MFX pair above.
2. Open a copy of the Sonic Classic 2 MFA in Fusion 2.5 build 295.
3. Use Fusion's Replace Object command to replace the original AssArray instance with **AssArray SC2 Compatible**.
4. Verify the four targeted actions and `GetValue` expression remain attached with the same IDs/parameters; do not convert the events manually as part of this task.
5. Run the game and check its existing load/save/GetValue behavior.

The new object has a different identity and does not replace the original `ASSARRAY` extension globally. The actual MFA conversion and game runtime have not been verified here.

## Runtime format boundaries

Windows and Android implement a numeric key/value subset: ASCII key lookup is normalized to lowercase, missing values return zero, and save output is a flat UTF-8 `key=value` file. Section headers and comment lines may be read, but section names are ignored. String values, addresses, encrypted/binary array files, list files, group-delimiter/exclusion options and unrelated ACEs are outside the five-operation target. `GetValue` does not report file errors; the inspected SC2 event set does not use AssArray conditions.

Case normalization and flat-file behavior follow this compatibility implementation; the original AssArray key-case behavior has not been independently established. This should not be described as full AssArray emulation.

The checked-in `ringdata.sav` is a Sonic Classic 1 sample with flat numeric keys. It is only a parser/round-trip fixture in host/CI tests and does **not** prove Sonic Classic 2 save compatibility. `SC2sf.ini` remains opaque and is not test evidence. No target-game save compatibility is claimed until a real SC2 save has been checked.

To obtain a target sample, run the Sonic Classic 2 build and trigger its existing Save INI event using the exact path configured in the target MFA. On Android, retrieve that actual file with Android Studio Device Explorer or `adb shell run-as <application-id>` for a debuggable build; alternatively capture it from a desktop Fusion run. Keep this SC2 sample separate from the SC1 fixture, then test load/save/GetValue against it.

## Android runtime package

The pure-Java runtime uses the same distinct `ASSARRAYSC2` identity:

- `android/java/Extensions/CRunAssArraySC2.java`
- `android/java/Extensions/AssArraySC2IniStore.java`

Package it with:

```sh
tools/package_assarray_sc2_android_extension.sh --out build/android-package
```

This creates `ASSARRAYSC2.zip`, for installation at `<Fusion 2.5>/Data/Runtime/Android/ASSARRAYSC2.zip`. For an extracted Fusion Android exporter:

```sh
tools/integrate_assarray_sc2_android_exporter.sh \
  --exporter /path/to/extracted-Fusion-295.10-project
```

The helper checks the known Fusion Java API surface and adds a `CExtLoad.java` branch for `ASSARRAYSC2` without replacing the vendor loader. A source/API guard is not a Java/Gradle compile. The package has not been verified by a Fusion 295.10 merge, target MFA export or device run; those steps remain required.

## Validation in this repository

Run:

```sh
python3 tools/check_assarray_sc2_extension.py
bash tools/run_host_tests.sh
SANITIZE=1 bash tools/run_host_tests.sh
```

The static guard checks the ACE IDs/signatures and distinct identity. Host tests compile/test the portable numeric store using Sonic Classic 1 `ringdata.sav`, and smoke-test the Android ZIP layout. CI run [42492164267](https://github.com/mymes1/IniPlusPlus/actions/runs/42492164267) passed Windows MFX compilation/export checks, host tests, the standalone Java store test and Android packaging. None of these checks establishes SC2 save compatibility or proves Replace Object on the target MFA.
