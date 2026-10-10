# Installing and using Ini++

For triggering builds and downloading artifacts from GitHub Actions, see the [GitHub Actions build and installation guide](GITHUB_ACTIONS.md). It explains which artifacts are usable and which Android acceptance steps remain manual.

## Windows / editor

Install the matching Windows MFX pair under your Fusion 2.5 Unicode installation:

- Edittime MFX: `<Fusion 2.5>/Extensions/Unicode/INI++.mfx`
- Runtime MFX: `<Fusion 2.5>/Data/Runtime/Unicode/INI++.mfx`

The Windows editor resources, icon, ACE definitions/order and serialized object data remain in the existing Windows project. Existing MFAs should continue to resolve the same extension; the MFX rebuild still needs to be run to establish non-regression for the current branch.

You can use `packaging/install-windows.bat` with the two built MFX files, or copy them manually. See [BUILD.md](BUILD.md) for the Visual Studio/MSBuild commands.

## Android: not a verified install yet

There is currently **no tested Android release ZIP**. The target is Fusion 2.5 build **295.10**, but the repository-root `AndroidSDK_Gradle.zip` identifies itself as Fusion 293.0. The integration helper refuses that mismatch. No build-295.10 Java/Gradle compile, exporter merge, Sonic MFA APK, or device test has been completed. Do not treat a source-only archive as a working extension.

The intended path is a reusable runtime integration, so a normal MFA should need no event changes once the Java adapter is compatible and installed. However, the current bridge does not marshal object references or Ini++ custom-parameter payloads, and four object-property ACEs are guarded as no-ops. That gap must be resolved or accepted for the particular MFA before the no-event-change criterion can be claimed; see [COMPATIBILITY.md](COMPATIBILITY.md).

### Build/integrate with the exact exporter

1. Obtain the authorized Fusion 2.5 build 295.10 Gradle exporter project and its documented Java/Gradle/Android SDK tool versions. Do not use the repository's 293.0 archive as a substitute.
2. Build the JNI library using Android NDK `26.1.10909125`:

   ```sh
   export ANDROID_NDK_HOME=/path/to/android-ndk-r26.1.10909125
   tools/build_android.sh --out build/android
   ```

3. Check and integrate the Java adapter into the exact exporter source tree:

   ```sh
   tools/integrate_android_exporter.sh \
     --exporter /path/to/Fusion-295.10-Gradle-project \
     --android-dir build/android \
     --check-only

   tools/integrate_android_exporter.sh \
     --exporter /path/to/Fusion-295.10-Gradle-project \
     --android-dir build/android \
     --assemble-debug
   ```

   The helper copies `CRunIniPlusPlus.java` and the generated ACE parameter table, copies `libIniPlusPlusBridge.so` under `app/src/main/assets/mmf/<ABI>/`, and adds explicit `CExtLoad.java` registration. Existing differing files are not overwritten. A successful Gradle build proves compilation only; it does not test an MFA export or device.

4. Use Fusion 2.5 build 295.10 to export the user's normal Sonic MFA with the Android exporter. Do not edit its Fusion events for this acceptance test. Select ABI(s) matching the device and follow the exporter project's signing/export procedure.
5. Inspect the APK contents to confirm the native library is present for the device ABI and install it on a device. Test a representative group/item/string/numeric/position ACE path, file load/save/autosave inside the app data directory, and every object/custom-parameter feature the MFA uses. Check `adb logcat -s IniPlusPlus` for JNI, CExtLoad or sandbox-path errors.
6. Compare the exported APK/runtime behavior against the existing Windows build and record the exact exporter, Java, Gradle, AGP and NDK versions. Only after this test passes should the Android ZIP/release be called complete.

`tools/package_android_extension.sh` can assemble an `INI++.zip` **overlay** after the three NDK binaries are built, but it is not evidence that Fusion's standard extension-install ZIP merge path or the 295.10 exporter accepts it. It intentionally contains no proprietary exporter/SDK files. See [BUILD.md](BUILD.md) for the output layout and limitations.

## Acceptance checklist

- [ ] Windows runtime and edittime MFX build and load in Fusion 2.5 Unicode.
- [ ] The user's existing Sonic MFA opens without event changes.
- [ ] The exact Fusion 2.5 build 295.10 Java/Gradle exporter compiles the adapter and JNI bridge.
- [ ] `CExtLoad` registration and `assets/mmf/<ABI>/libIniPlusPlusBridge.so` load in the exported APK.
- [ ] Sonic MFA exports to an APK and launches on a physical Android device.
- [ ] INI read/write/autosave, UTF-8 data, numeric/position expressions and compatibility with Windows-generated INI files are verified.
- [ ] Object-based ACEs and custom parameters used by the MFA behave acceptably without event changes.
- [ ] Any platform differences are documented before packaging a release.
