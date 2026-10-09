Ini++ (Unicode) @VERSION@
Fusion 2.5 / Multimedia Fusion 2 (Unicode) extension package
Packaged @DATE@

Android status
--------------
The Android runtime source in this project is not yet a verified Android release. The current
Gradle SDK package available to the project owner lacks Clickteam's proprietary RuntimeNative.h,
which is required to build the legacy native C++ extension. This archive's Android files must only
be treated as distributable if they were built against a complete, licensed Clickteam SDK; a mock
or shim-based smoke .so is not acceptable. See docs/BUILD.md, section 2, and docs/COMPATIBILITY.md.

What is in this archive
-----------------------
  windows\install.bat            installs the extension into a detected Fusion 2.5 / MMF2 install
  windows\edittime\INI++.mfx     edittime MFX (the editor loads this)
  windows\runtime\INI++.mfx      runtime MFX (applications and players load this)
  android\INI++.zip              Fusion Android extension package: copy it into
                                 <Fusion>\Data\Runtime\Android\ before building an APK
  android\assets\mmf\<abi>\      the Android natives that INI++.zip contains, for reference or for
    CRunINI++.so                 placing them into an Android project by hand
  docs\INSTALL.md                step-by-step installation and use, Windows and Android
  docs\COMPATIBILITY.md          what the Android build supports, and its limitations
  docs\BUILD.md                  how to build everything from source
  README.txt                     this file

Quick install - Windows
-----------------------
  1. Extract this archive (all files - install.bat needs the two folders next to it).
  2. Run windows\install.bat
     It finds Fusion 2.5 Developer/Standard and MMF2 Developer/Standard in the registry and copies
     the MFXs to:
        <Fusion>\Extensions\Unicode\INI++.mfx
        <Fusion>\Data\Runtime\Unicode\INI++.mfx
     If the detection fails, pass the folder yourself:
        windows\install.bat "C:\Program Files (x86)\Clickteam Fusion 2.5"
     ... or copy the two files by hand (docs\INSTALL.md, section 1).
  3. Restart Fusion. The object appears as "Ini++" (Unicode); existing projects that use it need no
     changes.

Quick install - Android (only for a verified release)
------------------------------------------------------
  1. Only proceed if this archive contains real INI++.so files built against the compatible,
     licensed Clickteam native C++ SDK; do not install a mock/shim build. Copy android\INI++.zip into
     <Fusion>\Data\Runtime\Android\ (the Android exporter merges it into the runtime project).
  2. Build the application for Android as usual. The Android exporter needs JDK 11, Android SDK
     Platform API 34 / Build-Tools 34.x, the NDK and Gradle - see docs\INSTALL.md, section 2.
  3. To verify: unzip the built APK and look for
        assets\mmf\arm64-v8a\CRunINI++.so   (and armeabi-v7a, x86_64)

Notes
-----
  * The intended Android port preserves the Windows object, ACEs, properties and INI file format.
    Compatibility with a real exporter/runtime and an unmodified MFA is not verified yet.
  * The Actions/Conditions/Expressions that are unimplemented in this fork (search, merge, MD5,
    CSV, charts, dialogs, ...) do nothing and log once on Android; on Windows they show an
    "unimplemented" message box. The full list is in docs\COMPATIBILITY.md.
  * Object-based ACEs ("Save/Load object properties") and Ini++'s own custom parameters cannot
    work on Android (the native extension API does not expose other objects); they are safe no-ops
    there. See docs\COMPATIBILITY.md, sections 3.2 and 3.3.
