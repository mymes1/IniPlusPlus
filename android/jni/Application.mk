# Android ABIs supported by the Fusion 2.5 exporter target. Package one shared library for every ABI
# selected in the application export settings so the Java runtime can load the JNI bridge.
APP_ABI := arm64-v8a armeabi-v7a x86_64

# Native minimum API for this build. Confirm/keep compatible with the exact exporter's APK minSdk.
APP_PLATFORM := android-21

# The C++20 shared core uses std::filesystem, exceptions and RTTI. Link libc++ statically because
# Fusion-generated APKs are not guaranteed to ship libc++_shared.so.
APP_STL := c++_static
APP_CPPFLAGS := -fexceptions -frtti
