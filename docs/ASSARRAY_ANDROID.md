# AssArray Android compatibility subset for Sonic Classic 2

## Scope and ACE mapping

The inspected Sonic Classic 2 event export uses five AssArray operations. This adapter targets those operations while leaving the MFA events and original Windows MFX files unchanged:

| Original ACE | ID from the supplied `AssArray.mfx` | Adapter |
| --- | ---: | --- |
| Set Value at Key | Action 0 | Implemented |
| Clear the Array | Action 3 | Implemented |
| Load INI File into Values | Action 12 | Implemented |
| Save INI File from Values | Action 14 | Implemented |
| `GetValue` | Expression 0 | Implemented |

It reports the original 12-condition table size, but all conditions return false; the inspected Sonic Classic 2 export does not use AssArray conditions. String values, address-based operations, binary array/list/encrypted formats, group-delimiter/exclusion options, and all other AssArray ACEs are outside this five-operation target.

## Save-file sample and compatibility boundary

The root `ringdata.sav` is from Sonic Classic 1, not Sonic Classic 2. It is readable flat text: one numeric `key=value` pair per CRLF line, with no section header (for example, lowercase `ring...=1` keys). The Sonic Classic 2 event expressions use uppercase `RING...`, so the adapter normalizes keys case-insensitively to lowercase.

CI uses this file only as a format fixture; it does **not** establish that the Sonic Classic 2 save has the same format or prove compatibility. `SC2sf.ini` remains opaque and is not used as test evidence. The unchanged MFA still needs to load/save on Android and be checked against a save from the actual target build.

To obtain a target fixture, run the Sonic Classic 2 build and let its existing Save INI action write `ringdata.sav` at the path used by the event export (`Appdrive$ + Appdir$ + "ringdata.sav"`). On Android, the app-private files directory can be inspected with Android Studio Device Explorer or `adb shell run-as <application-id>` when the build is debuggable. If that build does not expose its private files, use a desktop Fusion run or a separate diagnostic copy to save the same array to an accessible path; leave the shipped MFA events unchanged. Keep the resulting Sonic Classic 2 fixture separate from the Sonic Classic 1 root sample.

## Android package and integration

`android/java/Extensions/CRunAssArray.java` is a pure Java Fusion `CRunExtension` adapter. `AssArrayIniStore.java` handles numeric values, flat INI load/save, and app-private paths. No NDK library is required for this subset.

Package it with:

```sh
bash tools/package_assarray_android_extension.sh --out build/android-package
```

The script writes `ASSARRAY.zip`. An extracted Android exporter can be integrated with:

```sh
bash tools/integrate_assarray_android_exporter.sh --exporter /path/to/exporter
```

The helper copies the Java files and adds a `CExtLoad.java` registration branch without replacing the vendor loader. Its source checks were exercised against the bundled Fusion 293.0 API reference, not the target Fusion 295.10 exporter. A target 295.10 compile, unchanged MFA export, and device test are still required.

## Windows MFX build limitation

The repository contains only the prebuilt `AssArray.mfx` (editor-time) and `AssArrayruntime.mfx` (runtime) binaries. There is no AssArray `.sln`, `.vcxproj`, C/C++ source, or resource source in this checkout. A GitHub Actions workflow cannot compile new AssArray MFX files from those binaries; it could only validate or re-upload the existing files. The existing `windows-mfx` job builds the Ini++ source project, not AssArray.

To add a genuine AssArray MFX compile job, the repository needs the corresponding AssArray source project and its build dependencies/Clickteam SDK. Once that source is available, a Windows 2022 MSBuild job can be added alongside the existing Ini++ MFX job.
