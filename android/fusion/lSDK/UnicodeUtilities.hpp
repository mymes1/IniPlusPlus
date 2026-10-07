#ifndef lSDK_UnicodeUtilities_HeaderPlusPlus
#define lSDK_UnicodeUtilities_HeaderPlusPlus

/*
 * Ini++ Android runtime: string utilities.
 *
 * On Windows, Ini++ is built as a Unicode (UTF-16) extension: `TCHAR` is `wchar_t`, `TSL("...")`
 * is a wide literal and `lSDK::string_t` is `std::wstring`.  The Fusion 2.5 Android runtime hands
 * extension parameters to native code as NUL terminated UTF-8 `char *`, so in this build
 * `TCHAR == char`, `TSL("...")` is a plain narrow literal and `lSDK::string_t == std::string`
 * holding UTF-8.
 *
 * The functions below keep the exact names and signatures of the Windows version so that the
 * shared Ini++ sources (Runtime.cpp, ACEs.cpp, RunData.hpp, EditData.hpp) compile unchanged.
 * Where the Windows version converts between ANSI code pages and UTF-16, this version converts
 * between those same Windows code pages and UTF-8, using UTF-16 as the intermediate form:
 *
 *   - `wide_from_narrow(s, codepage)` treats `s` as bytes in `codepage` and returns UTF-8.
 *     With no code page (the common case) the bytes are already UTF-8 and are returned as-is.
 *   - `narrow_from_wide(s, codepage)` converts UTF-8 back to `codepage`.
 *     With no code page the UTF-8 bytes are returned as-is; this is also what the encryption
 *     feature hashes/writes, so encrypted files match the Windows build for ASCII content and
 *     use UTF-8 (rather than a Windows code page) for non-ASCII content (see COMPATIBILITY.md).
 *
 * Supported code pages: UTF-8 (65001), the Windows ANSI code pages 1252/28591 (ISO-8859-1) and
 * the UTF-16LE "code page" Windows uses for Unicode apps.  Unknown code pages fall back to
 * Windows-1252, which is what the Windows build effectively does for the default ANSI code page.
 */

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace lSDK
{
	#define NSL(s) u8##s
	#define WSL(s) L##s
	// Fusion 2.5 Android: the runtime's "TCHAR" is a UTF-8 byte string
	#define TSL(s) s

	template<typename NarrowType, typename WideType>
	using unicode_type = NarrowType;

	using string8_t  = std:: string; // UTF-8
	using string16_t = std::wstring; // UTF-16 (wchar_t is 32 bit on Android; content is still UTF-16)
	using string_t = string8_t;      // UTF-8, the extension's native string type on Android
	using char_t = string_t::value_type;
	using string_view8_t  = std:: string_view;
	using string_view16_t = std::wstring_view;
	using string_view_t = string_view8_t;

	// Convert bytes in `codepage` (Windows-1252 by default, UTF-8 when no code page is given) to the
	// extension's native string type.
	string_t  wide_from_narrow(string_view8_t , std::optional<std::uint32_t> codepage = std::nullopt);
	// Convert the extension's native string to bytes in `codepage` (UTF-8 when none is given).
	string8_t  narrow_from_wide(string_view16_t, std::optional<std::uint32_t> codepage = std::nullopt);
	// UTF-8 <-> UTF-16 helpers used when a true wide string is needed (Windows code page handling).
	string16_t utf16_from_native(string_view_t );
	string_t   native_from_utf16(string_view16_t);
	// On Android the extension's own strings are already UTF-8, so converting "wide" (the string_t
	// of the Windows build) is the identity; this overload keeps shared sources that pass string_t
	// to narrow_from_wide() compiling, exactly as they would on Windows where string_t is wide.
	string8_t  inline narrow_from_wide(string_view_t const s, std::optional<std::uint32_t> const /******/ = std::nullopt) { return string8_t{s}; }
	string_t   inline wide_from_narrow(string_view16_t const s, std::optional<std::uint32_t> const /******/ = std::nullopt) { return native_from_utf16(s); }

	string16_t inline wide_from(string_view16_t const s, std::optional<std::uint32_t> const /******/ = std::nullopt) { return string16_t{s}; }
	string16_t inline wide_from(string_view8_t const bytes, std::optional<std::uint32_t> const codepage = std::nullopt) { return utf16_from_native(wide_from_narrow(bytes, codepage)); }
	string8_t  inline narrow_from(string_view_t const s, std::optional<std::uint32_t> const /******/ = std::nullopt) { return string8_t{s}; }

	string_t inline string_t_from(string_view_t const s, std::optional<std::uint32_t> const /******/ = std::nullopt) { return string_t{s}; }
	string_t inline string_t_from(string_view16_t const s, std::optional<std::uint32_t> const codepage = std::nullopt) { return narrow_from_wide(s, codepage); }

	template<typename Numeric>
	auto string_t_from_numeric(Numeric const numeric) noexcept(noexcept(std::to_string(numeric)))
	-> string_t
	{
		return std::to_string(numeric);
	}

	template<typename CharacterTo, typename CharacterFrom>
	auto convert_to(std::basic_string_view<CharacterFrom> const s, [[maybe_unused]] std::optional<std::uint32_t> const codepage = std::nullopt)
	-> std::basic_string<CharacterTo>
	{
		static_assert(
			std::is_same_v<CharacterTo, CharacterFrom> ||
			std::is_same_v<CharacterTo, string8_t::value_type> ||
			std::is_same_v<CharacterTo, string16_t::value_type>
		);
		if constexpr(std::is_same_v<CharacterTo, CharacterFrom>)
		{
			return std::basic_string<CharacterTo>{s};
		}
		else if constexpr(std::is_same_v<CharacterTo, string8_t::value_type>)
		{
			return narrow_from_wide(utf16_from_native(s), codepage);
		}
		else if constexpr(std::is_same_v<CharacterTo, string16_t::value_type>)
		{
			return utf16_from_native(wide_from_narrow(s, codepage));
		}
	}
	template<typename CharacterTo, typename CharacterFrom>
	auto convert_to(std::basic_string<CharacterFrom> const s, std::optional<std::uint32_t> const codepage = std::nullopt)
	-> std::basic_string<CharacterTo>
	{
		return convert_to<CharacterTo, CharacterFrom>(std::basic_string_view<CharacterFrom>{s}, codepage);
	}
	template<typename CharacterTo>
	auto convert_to(string_t const &s, std::optional<std::uint32_t> const codepage = std::nullopt)
	-> std::basic_string<CharacterTo>
	{
		return convert_to<CharacterTo, string_t::value_type>(std::basic_string_view<string_t::value_type>{s}, codepage);
	}
}

#endif
