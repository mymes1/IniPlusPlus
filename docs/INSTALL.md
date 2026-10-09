# Installing and using Ini++

Two installation layouts are documented, depending on where the app runs. Windows installation is supported by the MFX artifacts. **Android installation is conditional and is not yet a verified release**: the C++ build is blocked pending a complete compatible Clickteam native SDK/header (see `docs/BUILD.md`, section 2). The intended Android package preserves the object/ACE surface and MFA data; do not treat it as working in an APK until the real `.so` is built and the user's MFA is tested.

## 0. From a packaged release (recommended)

When CI has produced a complete `release-bundle` (which is **blocked** until the Clickteam Android
C++ SDK/header is supplied), download `IniPlusPlus-<version>.zip` and extract it **completely** - the
installer needs the folders next to it. A stand-in/smoke build is not a distributable Android package.
Then:

* **Windows:** run `windows\install.bat`. It finds Fusion 2.5 Developer/Standard and MMF2
  Developer/Standard in the registry and copies both MFXs into place (sections 1 below, if you
  prefer doing it by hand or the detection fails - e.g. portable installs, where you pass the folder:
  `windows\install.bat "D:\Fusion 2.5"`).
* **Android (only after a verified release exists):** copy `android\INI++.zip` into `<Fusion>\Data\Runtime\Android\` (section 2).
* The archive also contains `docs\` and, when packaged with `--source`, the sources.

| | Windows build | Android build |
| --- | --- | --- |
| File to install | `INI++.mfx` (runtime) and `INI++_edittime.mfx` (edittime) | `INI++.zip` (`assets/mmf/<abi>/CRunINI++.so`) |
| Where it goes | into Fusion's extension folder / the app's data folder | into Fusion's `Data/Runtime/Android/` folder, merged into the APK by the Android exporter |
| Editor | Fusion 2.5 (Unicode) or MMF2 (Unicode) | Fusion 2.5 with the Android exporter; the MFA is still edited on Windows |

---

## 1. Windows (Fusion 2.5 / MMF2 Developer, Unicode)

1. Close Fusion.
2. Install the **edittime** MFX (the one built with `Edittime|Win32`, `INI++_edittime.mfx` in the CI artifacts) as `INI++.mfx` in the extension folder, so the editor can use the object:
   * Fusion 2.5: `<Fusion install>\Extensions\Unicode\INI++.mfx`
   * MMF2 Developer: `<MMF2 install>\Extensions\Unicode\INI++.mfx`
3. Install the **runtime** MFX (built with `Runtime|Win32`, `INI++.mfx` in the CI artifacts) as `INI++.mfx` in the runtime folder, so running and building applications works:
   * Fusion 2.5: `<Fusion install>\Data\Runtime\Unicode\INI++.mfx`
   * (Fusion copies that folder's extensions into the applications it builds.)
4. Start Fusion and open an MFA. The object appears in the object list as **Ini++** (Unicode).
4. Existing MFAs that already use Ini++ keep working: the edit data (properties) is read in the same format (`INI++` version 2 / Unicode) and older ANSI (version 1) data is still understood.

Both files must be named `INI++.mfx`; Fusion matches the edittime and runtime MFX by file name. If only the edittime MFX is installed, MFAs load but applications report a missing extension at run time, and vice versa.

---

## 2. Android (Fusion 2.5 + Android exporter)

> **Not currently verified:** these installation steps apply only after a genuine `INI++.zip` with
> `.so` files built against a complete compatible Clickteam C++ SDK has been produced. The currently
> available Gradle package lacks `RuntimeNative.h`; this branch has not exported the Sonic MFA to APK.

### 2.1 Prepare the exporter

Follow Clickteam's Android exporter setup (the extension adds nothing to it). In short, the machine needs:

* Fusion 2.5 with the **Android exporter** installed,
* **JDK 11** (Java 17+ is not supported by the exporter's Gradle/AGP combination),
* Android **SDK Platform API 34** and **Build-Tools 34.x**,
* the **Android NDK** that the exporter expects (the extension itself is built with NDK r23+, see `docs/BUILD.md`),
* Gradle (shipped with the exporter) and a device/emulator with developer mode.

### 2.2 Install the extension package

1. Close Fusion.
2. Copy `INI++.zip` into the Android exporter's runtime-extension folder:
   `<Fusion install>\Data\Runtime\Android\INI++.zip`
   (that folder is where Fusion looks for Android extensions; each ZIP is merged into the runtime project before the APK is built).
3. The ZIP contains one file per ABI:
   ```
   assets/mmf/arm64-v8a/CRunINI++.so
   assets/mmf/armeabi-v7a/CRunINI++.so
   assets/mmf/x86_64/CRunINI++.so
   ```
   If you build the extension yourself, use the layout from `tools/build_android.sh` / the artifacts of the CI workflow (`.github/workflows/main.yml`).
4. Start Fusion, open the MFA and build an Android application as usual.

### 2.3 Verify the installation

After the first APK build, unzip it (APKs are ZIP files) and check:

```sh
unzip -l MyApp.apk | grep CRunINI
#   assets/mmf/arm64-v8a/CRunINI++.so
#   assets/mmf/armeabi-v7a/CRunINI++.so
#   assets/mmf/x86_64/CRunINI++.so
```

At run time, if the object is missing, logcat shows `*** MISSING EXTENSION: INI++`; if the `.so` is there but fails to load, logcat shows the `dlopen`/`dlsym` error (see `docs/BUILD.md` section 7).

### 2.4 Using the object in events

Identical to Windows. The Android runtime hands the extension its edit data (the object properties from the MFA), so `Default file`, `Default text`, `Auto save`, `Case sensitive`, `Quote strings`, `Encrypt`, `Compress` and the rest keep behaving as configured. On first tick the object opens/creates its file the same way it does on Windows, only the *directory* changes: names without a path resolve inside the application's private data directory (`<Fusion app files dir>`), which is writable without permissions on every supported Android version:

* `Set current group` / `Set value` / `Set string` + `Save` write e.g. `settings.ini` into the app's data directory.
* `Load file` / `Save as` with a relative name resolve against the same directory, so an INI saved on one run is found on the next.
* Absolute paths (`/sdcard/...`, `/storage/...`) are used as given, but they need the permissions Android requires for those locations; the app's data directory is the recommended place.

### 2.5 Notes and limits specific to Android

* The ACEs that had no implementation on Windows either (search, merge, MD5, CSV, charts, dialogs, arrays, rename/move, ...) log `Ini++ (Android): ACE not implemented: <name>` once and do nothing instead of showing a message box. The full list is in `docs/COMPATIBILITY.md`.
* The four object-based save/load actions (`Save object properties`, `Load object properties` and
  their "in group" variants) cannot work on Android: the extension API does not expose other
  objects. On Android they log `Ini++ (Android): this ACE needs another object, ...` and do
  nothing - they never write bogus values. If your project uses them to persist a player/object
  state, write the values explicitly instead (`Set value "posx" = X of Sonic`, ...); see
  `docs/COMPATIBILITY.md` section 3.2.
* Ini++'s own custom parameters (`Ini++ parameter` in the ACE parameter list) cannot be read through the Android extension API; ACEs that use them see all-zero data. Avoid them in Android projects where the parameter changes behaviour.
* Encryption (`Encrypt`/`Toggle encryption`) uses the same Thayer cipher as Windows and stays compatible.

---

## 3. Upgrading / uninstalling

* **Upgrade:** replace `INI++.mfx` (Windows) and `INI++.zip` (Android) with the new versions; MFAs do not need to be changed. Rebuild the Android app so the new `.so` files are packaged.
* **Uninstall:** delete the files above. MFAs keep the Ini++ object data (they simply report the object as missing until it is installed again), and the INI files written by the extension are ordinary text files.
