// Ini++ Android runtime: code page conversions for the lSDK string utilities shim.
//
// The Windows build converts between the current ANSI code page (or, for Unicode apps, UTF-16)
// and TCHAR strings using MultiByteToWideChar/WideCharToMultiByte. Android has no such API, so
// this file implements the conversions Ini++ actually needs:
//
//   * UTF-8 <-> UTF-16, including surrogate pairs and replacement of malformed input
//   * Windows-1252 (and ISO-8859-1, which shares its 0xA0-0xFF layout) <-> UTF-16
//
// Windows-1252 is the default ANSI code page of the Western European Windows versions Ini++ is
// usually built for; unknown code pages fall back to it, matching the practical behaviour of the
// Windows build for the default code page.

#include "lSDK/UnicodeUtilities.hpp"

namespace
{
	constexpr std::uint32_t CP_UTF8{65001};
	constexpr std::uint32_t CP_UTF16LE{1200};
	constexpr char32_t REPLACEMENT_CHARACTER{0xFFFD};

	constexpr bool is_ascii(std::uint32_t const cp) noexcept { return cp < 0x80; }

	// The 0x80-0x9F range is where Windows-1252 differs from ISO-8859-1/Latin-1.
	constexpr char32_t cp1252_high[32]
	{
		0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
		0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
		0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178,
	};

	[[nodiscard]] constexpr char32_t from_cp1252(std::uint8_t const b) noexcept
	{
		if(b >= 0x80 && b <= 0x9F)
		{
			return cp1252_high[b - 0x80];
		}
		return b;
	}
	[[nodiscard]] constexpr std::uint8_t to_cp1252(char32_t const cp) noexcept
	{
		if(cp < 0x80 || (cp >= 0xA0 && cp <= 0xFF))
		{
			return static_cast<std::uint8_t>(cp);
		}
		for(std::uint8_t b{0x80}; b <= 0x9F; ++b)
		{
			if(cp1252_high[b - 0x80] == cp)
			{
				return b;
			}
		}
		return '?';
	}

	void append_utf8(std::string& out, char32_t const cp)
	{
		if(cp < 0x80)
		{
			out += static_cast<char>(cp);
		}
		else if(cp < 0x800)
		{
			out += static_cast<char>(0xC0 | (cp >> 6));
			out += static_cast<char>(0x80 | (cp & 0x3F));
		}
		else if(cp < 0x10000)
		{
			out += static_cast<char>(0xE0 | (cp >> 12));
			out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (cp & 0x3F));
		}
		else
		{
			out += static_cast<char>(0xF0 | (cp >> 18));
			out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
			out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (cp & 0x3F));
		}
	}

	// Decodes UTF-8 into UTF-16 code units; malformed sequences become U+FFFD.
	std::u16string utf8_to_utf16(std::string_view const s)
	{
		std::u16string out;
		out.reserve(s.size());
		for(std::size_t i{}; i < s.size();)
		{
			auto const b{static_cast<std::uint8_t>(s[i])};
			char32_t cp{};
			std::size_t len{};
			if(b < 0x80)                { cp = b;            len = 1; }
			else if((b & 0xE0) == 0xC0) { cp = b & 0x1F;     len = 2; }
			else if((b & 0xF0) == 0xE0) { cp = b & 0x0F;     len = 3; }
			else if((b & 0xF8) == 0xF0) { cp = b & 0x07;     len = 4; }
			else                        { out += u'\uFFFD'; ++i; continue; }
			if(i + len > s.size())
			{
				out += u'\uFFFD';
				break;
			}
			bool valid{true};
			for(std::size_t k{1}; k < len; ++k)
			{
				auto const cont{static_cast<std::uint8_t>(s[i + k])};
				if((cont & 0xC0) != 0x80)
				{
					valid = false;
					break;
				}
				cp = (cp << 6) | (cont & 0x3F);
			}
			if(!valid || (len == 2 && cp < 0x80) || (len == 3 && cp < 0x800) || (len == 4 && cp < 0x10000)
			|| (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF)
			{
				out += u'\uFFFD';
				++i;
				continue;
			}
			i += len;
			if(cp <= 0xFFFF)
			{
				out += static_cast<char16_t>(cp);
			}
			else
			{
				cp -= 0x10000;
				out += static_cast<char16_t>(0xD800 + (cp >> 10));
				out += static_cast<char16_t>(0xDC00 + (cp & 0x3FF));
			}
		}
		return out;
	}

	std::string utf16_to_utf8(std::u16string_view const s)
	{
		std::string out;
		out.reserve(s.size());
		for(std::size_t i{}; i < s.size(); ++i)
		{
			char32_t cp{s[i]};
			if(cp >= 0xD800 && cp <= 0xDBFF && i + 1 < s.size() && s[i + 1] >= 0xDC00 && s[i + 1] <= 0xDFFF)
			{
				cp = 0x10000 + ((cp - 0xD800) << 10) + (s[i + 1] - 0xDC00);
				++i;
			}
			else if(cp >= 0xD800 && cp <= 0xDFFF)
			{
				cp = REPLACEMENT_CHARACTER;
			}
			append_utf8(out, cp);
		}
		return out;
	}

	[[nodiscard]] bool is_utf8_codepage(std::optional<std::uint32_t> const codepage) noexcept
	{
		return !codepage || *codepage == CP_UTF8 || *codepage == 0;
	}

	std::u16string decode(std::string_view const bytes, std::optional<std::uint32_t> const codepage)
	{
		if(!codepage)
		{
			return utf8_to_utf16(bytes); // Fusion's own strings are UTF-8 on Android
		}
		switch(*codepage)
		{
			case CP_UTF8:
			case 0:
				return utf8_to_utf16(bytes);
			case CP_UTF16LE:
			case 1201: // UTF-16BE
			{
				std::u16string out;
				out.reserve(bytes.size() / 2);
				for(std::size_t i{}; i + 1 < bytes.size(); i += 2)
				{
					auto const lo{static_cast<std::uint8_t>(bytes[i])};
					auto const hi{static_cast<std::uint8_t>(bytes[i + 1])};
					out += *codepage == CP_UTF16LE ? static_cast<char16_t>(lo | (hi << 8)) : static_cast<char16_t>((lo << 8) | hi);
				}
				return out;
			}
			default: // Windows-1252 / ISO-8859-1 / unknown
			{
				std::u16string out;
				out.reserve(bytes.size());
				for(auto const c : bytes)
				{
					out += static_cast<char16_t>(from_cp1252(static_cast<std::uint8_t>(c)));
				}
				return out;
			}
		}
	}

	std::u16string to_utf16_from_wide(std::wstring_view const wide)
	{
		std::u16string utf16;
		utf16.reserve(wide.size());
		for(auto const c : wide)
		{
			auto const cp{static_cast<char32_t>(c)};
			if(cp <= 0xFFFF)
			{
				utf16 += static_cast<char16_t>(cp);
			}
			else
			{
				auto const v{cp - 0x10000};
				utf16 += static_cast<char16_t>(0xD800 + (v >> 10));
				utf16 += static_cast<char16_t>(0xDC00 + (v & 0x3FF));
			}
		}
		return utf16;
	}

	std::string encode(std::u16string_view const utf16, std::optional<std::uint32_t> const codepage)
	{
		if(is_utf8_codepage(codepage))
		{
			return utf16_to_utf8(utf16);
		}
		switch(*codepage)
		{
			case CP_UTF16LE:
			{
				std::string out;
				out.reserve(utf16.size() * 2);
				for(auto const c : utf16)
				{
					out += static_cast<char>(c & 0xFF);
					out += static_cast<char>((c >> 8) & 0xFF);
				}
				return out;
			}
			default:
			{
				std::string out;
				out.reserve(utf16.size());
				for(auto const c : utf16)
				{
					// encode as Windows-1252, replacing what cannot be represented
					if(c >= 0xD800 && c <= 0xDFFF)
					{
						out += '?';
						continue;
					}
					out += static_cast<char>(to_cp1252(c));
				}
				return out;
			}
		}
	}
}

namespace lSDK
{
	std::u16string to_utf16(std::string_view const bytes, std::optional<std::uint32_t> const codepage)
	{
		return decode(bytes, codepage);
	}

	string_t wide_from_narrow(string_view8_t const bytes, std::optional<std::uint32_t> const codepage)
	{
		return utf16_to_utf8(decode(bytes, codepage));
	}

	string8_t narrow_from_wide(string_view16_t const wide, std::optional<std::uint32_t> const codepage)
	{
		return encode(to_utf16_from_wide(wide), codepage);
	}

	string16_t utf16_from_native(string_view_t const utf8)
	{
		// One UTF-16 code unit per wchar_t: wchar_t is 16 bit on Windows and 32 bit on Android,
		// where string16_t nevertheless holds UTF-16 (see the header), so no reinterpret_cast of
		// the internal u16string buffer is allowed here.
		auto const utf16{decode(utf8, std::nullopt)};
		string16_t out;
		out.reserve(utf16.size());
		for(char16_t const unit : utf16)
		{
			out.push_back(static_cast<wchar_t>(unit));
		}
		return out;
	}

	string_t native_from_utf16(string_view16_t const wide)
	{
		return utf16_to_utf8(to_utf16_from_wide(wide));
	}

	string_t native_from_utf16(std::u16string_view const utf16)
	{
		return utf16_to_utf8(utf16);
	}
}
