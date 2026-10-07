#ifndef Windows_AndroidShim_HeaderPlusPlus
#define Windows_AndroidShim_HeaderPlusPlus

/*
 * Ini++ Android runtime: replacement for <Windows.h>.
 *
 * Only the handful of Win32 declarations the shared Ini++ sources mention are provided; they are
 * either declaration-level conveniences (types/macros) or stubs implemented in
 * android/runtime/AndroidRuntime.cpp. Nothing here calls Win32.
 */

#include "FusionAPI.hpp"
#include "lSDK/WindowsUtilities.hpp"

#endif
