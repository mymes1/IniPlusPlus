// Ini++ Android runtime: platform implementation.
//
// This file provides everything the shared sources (Runtime.cpp, ACEs.cpp, RunData.hpp) expect
// from the platform when compiled for Android instead of Windows:
//
//   * the ACE parameter frame (see ParamFrame.hpp), including the string pool used by temp_string()
//   * logging (there is no MessageBox on Android)
//   * the application's writable data directory and file I/O (mvLoadTextFile/mvSaveTextFile)
//   * SerializedEditData::deserialize(), byte compatible with the blob the Windows editor writes
//
// It contains no Fusion-runtime-specific code: the RuntimeFunctions bridge lives in
// android/jni/IniPlusPlusExtension.cc, so this file can also be compiled and unit tested with a
// host compiler (see android/tests).

#include "FusionAPI.hpp"
#include "EditData.hpp"
#include "RunData.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

namespace ipp_android
{
	namespace
	{
		ParamFrame& frame() noexcept
		{
			// Fusion calls an extension from its main thread; a dummy (non-constexpr, non-const)
			// frame keeps the accessors valid for the duration of every ACE call.
			static ParamFrame instance;
			return instance;
		}
	}

	ParamFrame& current_frame() noexcept
	{
		return frame();
	}

	void* empty_object() noexcept
	{
		// The Android extension API cannot deliver Fusion object references or the payload of
		// Ini++'s own custom parameters, so ACEs that ask for one receive this zeroed block
		// (documented defaults).  It is laid out like a RunObject, which makes the object ACEs
		// harmless: a zero headerObject has hoOEFlags == 0, so the shared code's alterable-value
		// reflection (ACEs.cpp's get_alterables()) returns empty spans and reads nothing - the ACE
		// does nothing instead of dereferencing a null pointer.  See docs/COMPATIBILITY.md.
		static std::array<std::byte, sizeof(RunObject) + sizeof(paramExt) + 64> const block{};
		return const_cast<std::byte*>(block.data());
	}

	void log(std::string_view const text) noexcept
	{
#ifdef __ANDROID__
		// Android: to logcat (tag "IniPlusPlus"); avoids depending on JNI here.
		static_cast<void>(std::fprintf(stderr, "IniPlusPlus: %.*s\n", static_cast<int>(text.size()), text.data()));
#else
		std::fputs("IniPlusPlus: ", stderr);
		std::fwrite(text.data(), 1, text.size(), stderr);
		std::fputc('\n', stderr);
#endif
	}

	std::filesystem::path data_dir()
	{
		// The Fusion 2.5 Android runtime gives every application a private directory and resolves
		// the application's file names against it; extension natives load from <dataDir>/libs
		// (see Runtime.Native.init in the runtime). There is no path API in the native extension
		// interface, so the sandbox directory is derived from the process name, with an override
		// for tests and for unusual setups:
		//   1. $INIPLUSPLUS_ANDROID_DATA_DIR
		//   2. /data/data/<package name from /proc/self/cmdline>
		//   3. the current working directory
		if(char const* const override_dir{std::getenv("INIPLUSPLUS_ANDROID_DATA_DIR")})
		{
			if(*override_dir)
			{
				return std::filesystem::path{override_dir};
			}
		}
		try
		{
			std::ifstream cmdline{"/proc/self/cmdline", std::ios::binary};
			std::string package;
			if(cmdline && std::getline(cmdline, package, '\0') && !package.empty())
			{
				std::filesystem::path const dir{std::filesystem::path{"/data/data"} / package};
				if(std::filesystem::exists(dir) || std::filesystem::exists(dir.parent_path()))
				{
					return dir;
				}
			}
		}
		catch(...)
		{
		}
		std::error_code ec{};
		auto const cwd{std::filesystem::current_path(ec)};
		return ec ? std::filesystem::path{"."} : cwd;
	}
}

// -------------------------------------------------------------------------------------------------
// Fusion's text file helpers, as used by Data::load()/Data::save() (Windows: mvLoadTextFile /
// mvSaveTextFile in Cncy.h).  They return malloc()ed memory so the Windows-style smart pointer in
// Runtime.cpp keeps working, and they read/write UTF-8 text, accepting the UTF-8, UTF-16 and ANSI
// BOMs the Windows version supports.
// -------------------------------------------------------------------------------------------------
namespace
{
	[[nodiscard]] std::filesystem::path resolve_path(TCHAR const* const filename)
	{
		// The Windows runtime resolves relative names against the application directory; the
		// Android sandbox directory plays that role here.
		std::filesystem::path path{filename ? filename : ""};
		if(!path.has_root_path())
		{
			path = ::ipp_android::data_dir() / path;
		}
		path.make_preferred();
		return path;
	}

	[[nodiscard]] bool read_whole_file(std::filesystem::path const& path, std::string& out)
	{
		std::ifstream ifs{path, std::ios::binary};
		if(!ifs || !ifs.seekg({}, std::ios::end))
		{
			return false;
		}
		auto const end_pos{ifs.tellg()};
		if(end_pos <= 0)
		{
			out.clear();
			return true;
		}
		out.assign(static_cast<std::size_t>(end_pos), '\0');
		if(!ifs.seekg({}, std::ios::beg))
		{
			return false;
		}
		return static_cast<bool>(ifs.read(std::data(out), static_cast<std::streamsize>(std::size(out))));
	}

	void strip_bom(std::string& bytes)
	{
		if(bytes.size() >= 3 && static_cast<std::uint8_t>(bytes[0]) == 0xEF && static_cast<std::uint8_t>(bytes[1]) == 0xBB && static_cast<std::uint8_t>(bytes[2]) == 0xBF)
		{
			bytes.erase(0, 3);
		}
	}

	// One UTF-16 code unit per wchar_t, like lSDK::string16_t on this platform.
	[[nodiscard]] lSDK::string16_t utf16_from_bytes(std::string const& bytes, std::size_t const offset, bool const big_endian)
	{
		lSDK::string16_t out;
		out.reserve((bytes.size() - offset) / 2);
		for(std::size_t i{offset}; i + 1 < bytes.size(); i += 2)
		{
			auto const first {static_cast<std::uint8_t>(bytes[i])};
			auto const second{static_cast<std::uint8_t>(bytes[i + 1])};
			out.push_back(static_cast<wchar_t>(big_endian ? ((first << 8) | second) : (first | (second << 8))));
		}
		return out;
	}

	// Whether the bytes are well-formed UTF-8; anything else has to be Windows-1252 (an ANSI file
	// written by a Windows tool), because Ini++'s own files are UTF-8.
	[[nodiscard]] bool is_valid_utf8(std::string const& bytes) noexcept
	{
		std::size_t i{};
		while(i < bytes.size())
		{
			auto const c{static_cast<std::uint8_t>(bytes[i])};
			std::size_t extra{};
			if(c < 0x80)                { extra = 0; }
			else if((c & 0xE0) == 0xC0) { extra = 1; }
			else if((c & 0xF0) == 0xE0) { extra = 2; }
			else if((c & 0xF8) == 0xF0) { extra = 3; }
			else                        { return false; }
			if(i + extra >= bytes.size())
			{
				return false;
			}
			for(std::size_t j{1}; j <= extra; ++j)
			{
				if((static_cast<std::uint8_t>(bytes[i + j]) & 0xC0) != 0x80)
				{
					return false;
				}
			}
			i += extra + 1;
		}
		return true;
	}

	[[nodiscard]] std::string decode_text(std::string const& bytes)
	{
		// UTF-16 BOMs, as written by Windows tools: convert to UTF-8
		if(bytes.size() >= 2 && static_cast<std::uint8_t>(bytes[0]) == 0xFF && static_cast<std::uint8_t>(bytes[1]) == 0xFE)
		{
			return lSDK::narrow_from_wide(utf16_from_bytes(bytes, 2, false), std::nullopt);
		}
		if(bytes.size() >= 2 && static_cast<std::uint8_t>(bytes[0]) == 0xFE && static_cast<std::uint8_t>(bytes[1]) == 0xFF)
		{
			return lSDK::narrow_from_wide(utf16_from_bytes(bytes, 2, true), std::nullopt);
		}
		// UTF-8 (Ini++'s own format on Android) passes through unchanged; a file that is not valid
		// UTF-8 can only be a Windows-1252 file, which is what the Windows runtime would have
		// decoded through the application's code page.
		if(is_valid_utf8(bytes))
		{
			return bytes;
		}
		return lSDK::wide_from_narrow(bytes, 1252);
	}
}

TCHAR* mvLoadTextFile(mv* const, TCHAR const* const filename, std::int32_t const encoding, std::int32_t const) noexcept
{
	try
	{
		std::string bytes;
		if(!read_whole_file(resolve_path(filename), bytes))
		{
			return nullptr;
		}
		if(encoding == CHARENC_DEFAULT)
		{
			strip_bom(bytes);
		}
		auto const text{decode_text(bytes)};
		if(auto const buffer{static_cast<char*>(std::malloc(std::size(text) + 1))})
		{
			std::memcpy(buffer, text.c_str(), std::size(text) + 1);
			return buffer;
		}
	}
	catch(...)
	{
	}
	return nullptr;
}

std::int32_t mvSaveTextFile(mv* const, TCHAR const* const filename, TCHAR const* const text, std::int32_t const) noexcept
{
	try
	{
		auto const path{resolve_path(filename)};
		if(!path.parent_path().empty())
		{
			std::error_code ec{};
			std::filesystem::create_directories(path.parent_path(), ec);
		}
		std::ofstream ofs{path, std::ios::binary | std::ios::trunc};
		if(!ofs)
		{
			return FALSE;
		}
		auto const sv{std::string_view{text ? text : ""}};
		ofs.write(std::data(sv), static_cast<std::streamsize>(std::size(sv)));
		return ofs ? TRUE : FALSE;
	}
	catch(...)
	{
		return FALSE;
	}
}

// -------------------------------------------------------------------------------------------------
// SerializedEditData::deserialize for Android.
//
// The edit data blob is written by the Windows editor (Edittime.cpp) and travels unchanged in the
// MFA/CCN: fixed layout, little endian, version 2 strings are NUL terminated UTF-16.  Version 1
// (Jax's ANSI version) uses fixed size ANSI arrays whose code page comes from the application.
// Keep this in sync with Edittime.cpp's serialize()/deserialize() when the format changes.
// -------------------------------------------------------------------------------------------------
namespace
{
	struct EndOfEditdata {};
	struct UnknownEditdata {};

	template<typename T>
	[[nodiscard]] auto read_int(std::span<std::byte const>& s) -> T
	{
		if(std::size(s) < sizeof(T))
		{
			throw EndOfEditdata{};
		}
		T t{};
		std::memcpy(&t, std::data(s), sizeof(T));
		s = s.subspan(sizeof(T));
		return t;
	}
	template<typename T>
	[[nodiscard]] auto read_bool(std::span<std::byte const>& s) -> T
	{
		switch(T const t{read_int<T>(s)})
		{
			case 0: return false;
			case 1: return true;
			default: throw UnknownEditdata{};
		}
	}
	// v1: fixed size ANSI array; v2: NUL terminated UTF-16LE (what TCHAR is on Windows)
	template<std::size_t Len>
	[[nodiscard]] auto read_str_v1(std::span<std::byte const>& s, std::optional<std::uint32_t> const codepage) -> lSDK::string_t
	{
		if(std::size(s) < Len)
		{
			throw EndOfEditdata{};
		}
		lSDK::string_view8_t const ret(reinterpret_cast<char const*>(std::data(s)), Len);
		s = s.subspan(Len);
		// 1252 is the ANSI code page Jax's version targeted; override with the app's code page.
		return lSDK::wide_from_narrow(ret.substr(0, ret.find('\0')), codepage.value_or(1252));
	}
	[[nodiscard]] auto read_str_v2(std::span<std::byte const>& s) -> lSDK::string_t
	{
		std::size_t units{};
		bool terminated{false};
		while(units * 2 + 1 < std::size(s))
		{
			if(s[units * 2] == std::byte{} && s[units * 2 + 1] == std::byte{})
			{
				terminated = true;
				break;
			}
			++units;
		}
		if(!terminated)
		{
			throw EndOfEditdata{};
		}
		lSDK::string_view8_t const bytes(reinterpret_cast<char const*>(std::data(s)), units * 2);
		s = s.subspan((units + 1) * 2);
		return lSDK::wide_from_narrow(bytes, 1200); // UTF-16LE
	}
}

auto SerializedEditData::deserialize(std::optional<std::uint32_t> const codepage) const
-> EdittimeSettings
{
	EdittimeSettings settings;
	if(eHeader.extSize < static_cast<std::int16_t>(sizeof(SerializedEditData))
	|| eHeader.extVersion < INIPP_V1_ANSI
	|| eHeader.extVersion > INIPP_V2_UNICODE)
	{
		// On Windows this shows a message box; here it is a log line and the object is reset.
		::ipp_android::log("Ini++: this MFA was saved with an unknown version of the Ini++ object; properties were reset");
		return settings;
	}
	std::span<std::byte const> raw_span(raw, static_cast<std::size_t>(eHeader.extSize) - offsetof(SerializedEditData, raw));
	try
	{
		settings.b_defaultFile = read_bool<std::uint8_t>(raw_span);
		settings.b_ReadOnly    = read_bool<std::uint8_t>(raw_span);
		if(eHeader.extVersion == INIPP_V1_ANSI){ settings.defaultFile = read_str_v1<260>(raw_span, codepage); }
		else                                   { settings.defaultFile = read_str_v2(raw_span); }
		if(eHeader.extVersion == INIPP_V1_ANSI){ raw_span = raw_span.subspan(2); }
		settings.defaultFolder = read_int<std::int32_t>(raw_span);
		if(eHeader.extVersion == INIPP_V1_ANSI){ settings.defaultText = read_str_v1<3000>(raw_span, codepage); }
		else                                   { settings.defaultText = read_str_v2(raw_span); }
		settings.bool_CanCreateFolders = read_bool<std::uint8_t>(raw_span);
		settings.bool_AutoSave         = read_bool<std::uint8_t>(raw_span);
		settings.bool_stdINI           = read_bool<std::uint8_t>(raw_span);
		settings.bool_compress         = read_bool<std::uint8_t>(raw_span);
		settings.bool_encrypt          = read_bool<std::uint8_t>(raw_span);
		if(eHeader.extVersion == INIPP_V1_ANSI){ settings.encrypt_key = read_str_v1<128>(raw_span, codepage); }
		else                                   { settings.encrypt_key = read_str_v2(raw_span); }
		settings.bool_newline = read_bool<std::uint8_t>(raw_span);
		if(eHeader.extVersion == INIPP_V1_ANSI){ settings.newline = read_str_v1<10>(raw_span, codepage); }
		else                                   { settings.newline = read_str_v2(raw_span); }
		settings.bool_QuoteStrings = read_bool<std::uint8_t>(raw_span);
		if(eHeader.extVersion == INIPP_V1_ANSI){ raw_span = raw_span.subspan(3); }
		settings.repeatGroups = read_int<std::int32_t>(raw_span);
		settings.repeatItems  = read_int<std::int8_t>(raw_span);
		settings.undoCount    = read_int<std::int8_t>(raw_span);
		settings.redoCount    = read_int<std::int8_t>(raw_span);
		if(eHeader.extVersion == INIPP_V1_ANSI){ std::ignore = read_int<std::int8_t>(raw_span); }
		settings.saveRepeated       = read_bool<std::uint8_t>(raw_span);
		settings.bool_EscapeGroup   = read_bool<std::uint8_t>(raw_span);
		settings.bool_EscapeItem    = read_bool<std::uint8_t>(raw_span);
		settings.bool_EscapeValue   = read_bool<std::uint8_t>(raw_span);
		settings.bool_CaseSensitive = read_bool<std::uint8_t>(raw_span);
		settings.globalObject       = read_bool<std::uint8_t>(raw_span);
		settings.index              = read_bool<std::uint8_t>(raw_span);
		settings.autoLoad           = read_bool<std::uint8_t>(raw_span);
		settings.subGroups          = read_bool<std::uint8_t>(raw_span);
		settings.allowEmptyGroups   = read_bool<std::uint8_t>(raw_span);
		if(eHeader.extVersion == INIPP_V1_ANSI){ settings.globalKey = read_str_v1<32>(raw_span, codepage); }
		else                                   { settings.globalKey = read_str_v2(raw_span); }
		if(eHeader.extVersion == INIPP_V1_ANSI){ raw_span = raw_span.subspan(2); }
	}
	catch(...)
	{
		::ipp_android::log("Ini++: corrupt object data; properties were reset");
	}
	return settings;
}
