# ABIs the Fusion 2.5 Android exporter builds for (Runtime/MMFRuntime.java looks up
# assets/mmf/<abi>/, where <abi> comes from Build.CPU_ABI at run time).  Build the extension for
# every ABI the APK ships: a missing one means "*** MISSING EXTENSION: INI++" on that device.
APP_ABI := arm64-v8a armeabi-v7a x86_64

# clickteam's Android runtime supports API 21 and up (android-21 = Android 5.0).
APP_PLATFORM := android-21

# Static libc++: the Fusion-generated APK does not ship libc++_shared.so.
APP_STL := c++_static

# Exceptions and RTTI are needed by the shared Ini++ implementation (std::filesystem,
# std::optional, the parser); the official SDK template's default -fno-exceptions/-fno-rtti does
# not apply to this extension.
APP_CPPFLAGS := -fexceptions -frtti
