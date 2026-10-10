Ini++ (Unicode) release candidate

Android status: UNVERIFIED. The Android Java/JNI source has host-tested C++ bridge logic, but has not
been compiled against the exact Fusion 2.5 build 295.10 Gradle exporter, merged/exported with Fusion,
used to export the Sonic MFA, or run on a device. See docs/BUILD.md, docs/INSTALL.md and
ANDROID-STATUS.txt in the release candidate.

The repository's root AndroidSDK_Gradle.zip identifies as Fusion 292.0, not the selected 295.10
exporter. Do not substitute/repackage it. Generated runtime packages contain only Ini++-owned source
and binaries, never proprietary Clickteam SDK/exporter files. The standard extension ZIP merge path
is not yet verified.

Windows installation (after building the MFX files):
  * windows/edittime/INI++.mfx -> <Fusion>/Extensions/Unicode/INI++.mfx
  * windows/runtime/INI++.mfx -> <Fusion>/Data/Runtime/Unicode/INI++.mfx
  * Run windows/install.bat or follow docs/INSTALL.md.

Android integration requires the exact authorized build 295.10 Gradle exporter, NDK r26.1.10909125,
and its compatible Java/Gradle/JDK toolchain. Use tools/integrate_android_exporter.sh from the
source repository to add CRunIniPlusPlus registration and runtime files. Do not treat this candidate
or an incomplete overlay as a completed Android release until the Sonic MFA APK and device tests pass.
