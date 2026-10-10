# Installing and using Ini++

For triggering builds and downloading artifacts from GitHub Actions, see the [GitHub Actions build and installation guide](GITHUB_ACTIONS.md). It explains which artifacts are usable and which Android acceptance steps remain manual.

## Windows / editor

Install the matching Windows MFX pair under your Fusion 2.5 Unicode installation:

- Edittime MFX: `<Fusion 2.5>/Extensions/Unicode/INI++.mfx`
- Runtime MFX: `<Fusion 2.5>/Data/Runtime/Unicode/INI++.mfx`

The Windows editor resources, icon, ACE definitions/order and serialized object data remain in the existing Windows project. Existing MFAs should continue to resolve the same extension; the MFX rebuild still needs to be run to establish non-regression for the current branch.

You can use `packaging/install-windows.bat` with the two built MFX files, or copy them manually. If the release package includes `android/INI++.zip`, the same installer copies it to `<Fusion>/Data/Runtime/Android/INI++.zip`. See [BUILD.md](BUILD.md) for the Visual Studio/MSBuild commands.

## Android: source ZIP, not a verified install yet

The Java adapter is written against the Fusion 2.5 Java `CRunExtension` API surface in the available Fusion **293.0** Android runtime sources. The intended package is raw Java source plus Ini++-owned JNI `.so` files in the standard runtime-merge ZIP layout, installed as `<Fusion>/Data/Runtime/Android/INI++.zip`. The Fusion 295.10 exporter is intended to merge and compile those sources using its own build settings. The package contains no precompiled Java `.class`, `.jar` or `.aar` dependencies and no direct AndroidX imports.

This v293-to-v295.10 source-compatibility route has not yet been confirmed by a Fusion 295.10 merge/compile/export/device test. A static API check against 293.0 does not prove that the exporter automatically registers the class or accepts the package. The Java class must be registered in the target exporter's `CExtLoad` flow; do not overwrite its vendor-owned loader with a copied 293.0 file. The current bridge also does not marshal object references or Ini++ custom-parameter payloads, and four object-property ACEs are guarded as no-ops. Check whether the normal Sonic MFA uses those features before claiming it exports without event changes; see [COMPATIBILITY.md](COMPATIBILITY.md).

### Build and install the Fusion extension ZIP

1. Build the project-owned JNI libraries with Android NDK `26.1.10909125`:

   ```sh
   export ANDROID_NDK_HOME=/path/to/android-ndk-r26.1.10909125
   python3 tools/gen_android_aces.py --check
   tools/build_android.sh --out build/android
   ```

2. Package the source and ABI libraries. To copy directly into a Fusion installation's Android runtime directory:

   ```sh
   tools/package_android_extension.sh \
     --android-dir build/android \
     --out build/android-package \
     --install-to "/path/to/Fusion/Data/Runtime/Android"
   ```

   The resulting archive has `src/Extensions/CRunIniPlusPlus.java`, `src/Extensions/IniPlusPlusAceParams.java`, and `assets/mmf/<ABI>/libIniPlusPlusBridge.so` at its root. This follows the Clickteam Android extension merge layout. It excludes the proprietary root `AndroidSDK_Gradle.zip`, JAR/AAR dependencies, and exporter Gradle configuration. The script also writes `CExtLoad-registration.patch` next to the ZIP as a manual registration reference, not as a replacement for the target exporter's source file. It refuses to overwrite an existing `INI++.zip` unless you have made a backup and pass `--replace-installed`.

3. Build an unchanged Sonic MFA with Fusion 2.5 build 295.10. Verify that the exporter merged the Java files, registered `CRunIniPlusPlus` in its `CExtLoad` flow, compiled the sources, and included the proper ABI `.so` in the APK. If a direct extracted-project check is needed, after building the NDK libraries run:

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

   That developer helper checks the Java API surface and can compile the extracted project when its own JDK/Gradle/SDK dependencies are available. A green source check or APK compile alone does not prove a Fusion MFA export or device behavior.

4. Install the APK on a device. Test representative group/item/string/numeric/position ACEs, file load/save/autosave in the app-private files directory, UTF-8 and Windows-generated INI data, and every object/custom-parameter feature used by the MFA. Check `adb logcat -s IniPlusPlus` for JNI, loader or sandbox errors. Compare behavior against the Windows build and record the Fusion, Java, Gradle, AGP and NDK versions.

Only after the target exporter merges and compiles the source ZIP, the unchanged MFA exports and the device tests pass should the Android package be called complete.

## Acceptance checklist

- [ ] Windows runtime and edittime MFX build and load in Fusion 2.5 Unicode.
- [ ] The user's existing Sonic MFA opens without event changes.
- [ ] Fusion 2.5 build 295.10 merges and Java-compiles the raw source ZIP using its own toolchain.
- [ ] The target exporter's `CExtLoad` flow registers `CRunIniPlusPlus`; `assets/mmf/<ABI>/libIniPlusPlusBridge.so` loads.
- [ ] Sonic MFA exports to an APK and launches on a physical Android device.
- [ ] INI read/write/autosave, UTF-8 data, numeric/position expressions and compatibility with Windows-generated INI files are verified.
- [ ] Object-based ACEs and custom parameters used by the MFA behave acceptably without event changes.
- [ ] Any platform differences are documented before packaging a release.
