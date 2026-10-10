#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace ipp_android::bridge
{
	enum class ValueKind : std::uint8_t
	{
		Number,
		Integer,
		String,
		Object,
	};

	// Values evaluated by the public Java CRunExtension API. The JNI boundary translates Java
	// Float/Integer/String/null objects into these POD-like values; this bridge then feeds the
	// existing shared ACE implementation without depending on Clickteam's private C++ ABI.
	struct InputValue final
	{
		ValueKind kind{ValueKind::Object};
		double number{};
		std::int32_t integer{};
		std::string text;

		[[nodiscard]] static InputValue from_number(double value);
		[[nodiscard]] static InputValue from_integer(std::int32_t value);
		[[nodiscard]] static InputValue from_string(std::string value);
		[[nodiscard]] static InputValue object();
	};

	struct ExpressionResult final
	{
		enum class Kind : std::uint8_t { Integer, Float, String };
		Kind kind{Kind::Integer};
		std::int32_t integer{};
		float floating{};
		std::string text;
	};

	struct Extension;

	[[nodiscard]] int condition_count() noexcept;
	[[nodiscard]] Extension* create(std::span<std::byte const> edit_data, int edit_data_version,
	                                std::filesystem::path files_directory) noexcept;
	void destroy(Extension* extension, bool fast) noexcept;
	[[nodiscard]] int handle(Extension* extension) noexcept;
	void action(Extension* extension, int id, std::span<InputValue const> parameters) noexcept;
	[[nodiscard]] bool condition(Extension* extension, int id,
	                             std::span<InputValue const> parameters) noexcept;
	[[nodiscard]] ExpressionResult expression(Extension* extension, int id,
	                                          std::span<InputValue const> parameters) noexcept;
}
