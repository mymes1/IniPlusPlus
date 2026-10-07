#ifndef Shlobj_AndroidShim_HeaderPlusPlus
#define Shlobj_AndroidShim_HeaderPlusPlus

/*
 * Ini++ Android runtime: replacement for the Windows <Shlobj.h>.
 *
 * Runtime.cpp uses SHGetKnownFolderPath() to resolve the "default folder" property
 * (Windows / Program / AppData / ...).  Android apps have no such folders: every option resolves
 * to the application's sandbox data directory, which is the only location guaranteed to be
 * writable on all Android versions and the location the Fusion 2.5 Android runtime uses for
 * application files.  See docs/COMPATIBILITY.md.
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

namespace ipp_android
{
	[[nodiscard]] std::filesystem::path data_dir(); // android/runtime/AndroidRuntime.cpp
}

struct KNOWNFOLDERID final
{
	char const* name;
};
inline KNOWNFOLDERID const FOLDERID_Windows{"Windows"};
inline KNOWNFOLDERID const FOLDERID_RoamingAppData{"AppData"};
inline KNOWNFOLDERID const FOLDERID_ProgramData{"ProgramData"};

inline void CoTaskMemFree(void* const p) noexcept { std::free(p); }

// Allocates the app data directory (free with CoTaskMemFree, as on Windows).
inline long SHGetKnownFolderPath(KNOWNFOLDERID const&, std::uint32_t, void*, char** const out) noexcept
{
	if(!out)
	{
		return -1;
	}
	auto const dir{::ipp_android::data_dir().string()};
	*out = static_cast<char*>(std::malloc(dir.size() + 1));
	if(*out)
	{
		std::memcpy(*out, dir.c_str(), dir.size() + 1);
	}
	return *out ? 0 : -1;
}

#endif
