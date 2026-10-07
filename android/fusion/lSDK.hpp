#ifndef lSDK_HeaderPlusPlus
#define lSDK_HeaderPlusPlus

/*
 * Ini++ Android runtime: minimal replacement for lSDK.hpp.
 *
 * The Windows lSDK.hpp is header-only and platform independent except for the string utilities it
 * pulls in; this shim keeps the same API surface that the shared Ini++ runtime sources use
 * (`lSDK::fill_buffer`, `lSDK::FusionAceType`) on top of the Android string shim.
 */

#include "lSDK/UnicodeUtilities.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace lSDK
{
	template<typename CharFrom, typename CharTo>
	bool fill_buffer(CharTo* const buffer, std::intmax_t const max_len, std::basic_string_view<CharFrom> const& str)
	{
		if(!buffer)
		{
			throw std::runtime_error{"Pointer to buffer is null!"};
		}
		if(max_len > 0)
		{
			buffer[0] = TSL('\0');
		}
		if(max_len < 0)
		{
			throw std::runtime_error{"Negative size for buffer: " + std::to_string(max_len)};
		}
		if(max_len == 0)
		{
			return str.empty();
		}
		std::size_t const max_size = static_cast<std::size_t>(max_len - 1);
		auto const tstr = convert_to<CharTo>(str);
		auto const begin = std::cbegin(tstr);
		auto const end = begin + std::min(tstr.length(), max_size);
		*std::copy(begin, end, buffer) = 0;
		return tstr.length() <= max_size;
	}
	template<typename CharFrom, typename CharTo>
	bool fill_buffer(CharTo* const buffer, std::intmax_t const max_len, CharFrom const* const str)
	{
		return fill_buffer<CharFrom, CharTo>(buffer, max_len, static_cast<std::basic_string_view<CharFrom>>(str));
	}
	template<typename CharFrom, typename CharTo>
	bool fill_buffer(CharTo* const buffer, std::intmax_t const max_len, std::basic_string<CharFrom> const& str)
	{
		return fill_buffer<CharFrom, CharTo>(buffer, max_len, static_cast<std::basic_string_view<CharFrom>>(str));
	}

	enum struct FusionAceType
	{
		Action,
		Condition,
		Expression
	};

	// Android has no extension resources; the runtime/edittime code that reads them is not built
	// for this platform (see docs/COMPATIBILITY.md). This exists so shared code still compiles.
	inline auto get_resource_string(::std::uint32_t) -> string_view_t
	{
		return string_view_t{};
	}
}

#endif
