#ifndef lSDK_WindowsUtilities_HeaderPlusPlus
#define lSDK_WindowsUtilities_HeaderPlusPlus

/*
 * Ini++ Android runtime: replacement for lSDK/WindowsUtilities.hpp.
 *
 * The Windows version wraps <Windows.h> (FormatMessage, handles, LoadString, ...), none of which
 * exist on Android. The shared Ini++ runtime sources only include it for the edittime resource
 * helpers, which are not built for Android (see docs/COMPATIBILITY.md), so this header only
 * provides the string type aliases plus a couple of compile-time helpers.
 */

#include "lSDK/UnicodeUtilities.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace lSDK
{
	template<typename Func, typename Handle = void*>
	struct HandleDeleter final
	{
		using pointer = Handle;
		void operator()(pointer p) const noexcept
		{
			Func{}(p);
		}
	};
	template<typename Func, typename Handle = void*, bool /******/ = false>
	using unique_handle = std::unique_ptr<Handle, HandleDeleter<Func, Handle>>;

	struct WindowsError final
	: std::runtime_error
	{
		std::uint32_t error;
		WindowsError(std::uint32_t const error_, string_view_t const& message)
		: std::runtime_error{narrow_from(string_t{message}) + ": " + string_t_from_numeric(error_)}
		, error{error_}
		{
		}
	};
}

#endif
