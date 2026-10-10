Ini++ (Unicode) release candidate

Android status: UNVERIFIED. The Android extension ZIP contains Ini++ Java sources and project-owned
JNI libraries in the Data/Runtime/Android merge layout. The adapter is based on the Fusion 293.0
Java CRunExtension API surface and is intended for source compilation by the Fusion 2.5 build 295.10
exporter. That exact merge/registration/Gradle compile, unchanged Sonic MFA export and device test
have not yet been completed. See docs/BUILD.md, docs/INSTALL.md and ANDROID-STATUS.txt in the release
candidate.

The proprietary root AndroidSDK_Gradle.zip is never copied into generated packages. The extension
ZIP has no precompiled .class/.jar/.aar dependency and no direct AndroidX imports. It includes only
Ini++-owned .java sources and (after an NDK build) Ini++-owned .so files; it does not include
Clickteam exporter/runtime source.

Windows installation (after building the MFX files):
  * windows/edittime/INI++.mfx -> <Fusion>/Extensions/Unicode/INI++.mfx
  * windows/runtime/INI++.mfx -> <Fusion>/Data/Runtime/Unicode/INI++.mfx
  * android/INI++.zip -> <Fusion>/Data/Runtime/Android/INI++.zip
  * Run windows/install.bat or follow docs/INSTALL.md.

Android native build uses NDK r26.1.10909125. Fusion 295.10 must merge the source ZIP, register
CRunIniPlusPlus in its CExtLoad flow, and compile it with the exporter's own toolchain. Do not treat
this candidate or a package-only test as a completed Android release until the unchanged Sonic MFA
APK and device tests pass.
