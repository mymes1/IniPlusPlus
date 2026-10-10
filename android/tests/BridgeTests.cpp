// Host tests for the Java/JNI-independent Android runtime bridge.
//
// These compile the shared C++ ACE implementation plus Bridge.cpp against the small declaration
// shim in android/fusion. They test edit-data normalization, parameter order/types, object state,
// expression results, position packing, autosave, Unicode bytes, and sandbox path confinement.
// They do NOT load a Fusion exporter, compile JniBridge.cpp, or prove CExtLoad/APK integration.

#include "Bridge.hpp"
#include "AndroidRuntime.hpp"
#include "lSDK/UnicodeUtilities.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	using ipp_android::bridge::ExpressionResult;
	using ipp_android::bridge::InputValue;
	using ipp_android::bridge::ValueKind;

	int failures{};

	void check(bool const condition, std::string_view const message)
	{
		std::cout << (condition ? "  ok   " : "FAIL  ") << message << '\n';
		if(!condition)
		{
			++failures;
		}
	}

	void put_u8(std::vector<std::byte>& bytes, std::uint8_t const value)
	{
		bytes.push_back(static_cast<std::byte>(value));
	}

	void put_i8(std::vector<std::byte>& bytes, std::int8_t const value)
	{
		bytes.push_back(static_cast<std::byte>(value));
	}

	void put_i32(std::vector<std::byte>& bytes, std::int32_t const value)
	{
		auto const bits{static_cast<std::uint32_t>(value)};
		for(unsigned shift{}; shift < 32; shift += 8)
		{
			bytes.push_back(static_cast<std::byte>((bits >> shift) & 0xffu));
		}
	}

	void put_v2_string(std::vector<std::byte>& bytes, std::string_view const value)
	{
		auto const utf16{lSDK::utf16_from_native(value)};
		for(auto const code_unit : utf16)
		{
			auto const unit{static_cast<std::uint16_t>(code_unit)};
			bytes.push_back(static_cast<std::byte>(unit & 0xffu));
			bytes.push_back(static_cast<std::byte>((unit >> 8) & 0xffu));
		}
		bytes.push_back(std::byte{});
		bytes.push_back(std::byte{});
	}

	// The Java CRunExtension receives only CObjectCommon.ocExtension (the bytes after extHeader)
	// and the separate extVersion field. Construct the exact v2 payload Edittime.cpp serializes.
	std::vector<std::byte> make_v2_payload(std::string_view const default_file, bool const auto_save = true)
	{
		std::vector<std::byte> payload;
		put_u8(payload, 1); // b_defaultFile
		put_u8(payload, 0); // b_ReadOnly
		put_v2_string(payload, default_file);
		put_i32(payload, 2); // defaultFolder; on Android every option resolves to filesDir
		put_v2_string(payload, ""); // defaultText
		put_u8(payload, 1); // bool_CanCreateFolders
		put_u8(payload, auto_save ? 1 : 0);
		put_u8(payload, 1); // bool_stdINI
		put_u8(payload, 0); // bool_compress
		put_u8(payload, 0); // bool_encrypt
		put_v2_string(payload, ""); // encrypt_key
		put_u8(payload, 0); // bool_newline
		put_v2_string(payload, ""); // newline
		put_u8(payload, 0); // bool_QuoteStrings
		put_i32(payload, 3); // repeatGroups
		put_i8(payload, 1); // repeatItems
		put_i8(payload, 10); // undoCount
		put_i8(payload, 10); // redoCount
		put_u8(payload, 0); // saveRepeated
		put_u8(payload, 0); // bool_EscapeGroup
		put_u8(payload, 0); // bool_EscapeItem
		put_u8(payload, 0); // bool_EscapeValue
		put_u8(payload, 0); // bool_CaseSensitive
		put_u8(payload, 0); // globalObject
		put_u8(payload, 0); // index
		put_u8(payload, 0); // autoLoad
		put_u8(payload, 0); // subGroups
		put_u8(payload, 1); // allowEmptyGroups
		put_v2_string(payload, ""); // globalKey
		return payload;
	}

	void put_fixed_v1_string(std::vector<std::byte>& bytes, std::size_t const length,
	                         std::string_view const value)
	{
		for(std::size_t index{}; index < length; ++index)
		{
			bytes.push_back(index < value.size() ? static_cast<std::byte>(static_cast<unsigned char>(value[index])) : std::byte{});
		}
	}

	std::vector<std::byte> make_v1_payload(std::string_view const default_file, std::string_view const default_text)
	{
		std::vector<std::byte> payload;
		put_u8(payload, 1); // b_defaultFile
		put_u8(payload, 0); // b_ReadOnly
		put_fixed_v1_string(payload, 260, default_file);
		payload.insert(payload.end(), 2, std::byte{}); // old alignment
		put_i32(payload, 2); // defaultFolder
		put_fixed_v1_string(payload, 3000, default_text);
		put_u8(payload, 1); // bool_CanCreateFolders
		put_u8(payload, 1); // bool_AutoSave
		put_u8(payload, 1); // bool_stdINI
		put_u8(payload, 0); // bool_compress
		put_u8(payload, 0); // bool_encrypt
		put_fixed_v1_string(payload, 128, ""); // encrypt_key
		put_u8(payload, 0); // bool_newline
		put_fixed_v1_string(payload, 10, ""); // newline
		put_u8(payload, 0); // bool_QuoteStrings
		payload.insert(payload.end(), 3, std::byte{}); // old alignment
		put_i32(payload, 3); // repeatGroups
		put_i8(payload, 1); // repeatItems
		put_i8(payload, 10); // undoCount
		put_i8(payload, 10); // redoCount
		put_i8(payload, 0); // old placeholder
		put_u8(payload, 0); // saveRepeated
		put_u8(payload, 0); // bool_EscapeGroup
		put_u8(payload, 0); // bool_EscapeItem
		put_u8(payload, 0); // bool_EscapeValue
		put_u8(payload, 0); // bool_CaseSensitive
		put_u8(payload, 0); // globalObject
		put_u8(payload, 0); // index
		put_u8(payload, 0); // autoLoad
		put_u8(payload, 0); // subGroups
		put_u8(payload, 1); // allowEmptyGroups
		put_fixed_v1_string(payload, 32, ""); // globalKey
		payload.insert(payload.end(), 2, std::byte{}); // old alignment
		return payload;
	}

	InputValue string(std::string value) { return InputValue::from_string(std::move(value)); }
	InputValue number(double value) { return InputValue::from_number(value); }
	InputValue integer(std::int32_t value) { return InputValue::from_integer(value); }

	void run_runtime_tests(std::filesystem::path const& sandbox)
	{
		std::filesystem::create_directories(sandbox);
		auto payload{make_v2_payload("nested/game.ini")};
		auto* const extension{ipp_android::bridge::create(payload, 2, sandbox)};
		check(extension != nullptr, "create a runtime object from the Java payload-only v2 edit data");
		if(!extension)
		{
			return;
		}

		check(ipp_android::bridge::condition_count() == 20, "condition count matches the checked-in Ini++ ACE declarations");

		ipp_android::bridge::action(extension, 0, std::vector<InputValue>{string("Game"), number(0)});
		ipp_android::bridge::action(extension, 15, std::vector<InputValue>{string("Game"), string("Welcome"), string("Hello, café 🐟")});
		auto const text{ipp_android::bridge::expression(extension, 9,
			std::vector<InputValue>{string("Game"), string("Welcome"), string("fallback")})};
		check(text.kind == ExpressionResult::Kind::String && text.text == "Hello, café 🐟",
			"string ACE parameters and UTF-8 expression results round-trip");

		auto const has_item{ipp_android::bridge::condition(extension, 4,
			std::vector<InputValue>{string("Game"), string("Welcome")})};
		check(has_item, "condition dispatch reads ordered string parameters");

		ipp_android::bridge::action(extension, 14, std::vector<InputValue>{
			string("Game"), string("Score"), number(1), number(3.5)});
		auto const score{ipp_android::bridge::expression(extension, 8,
			std::vector<InputValue>{string("Game"), string("Score"), integer(0)})};
		check(score.kind == ExpressionResult::Kind::Float && score.floating == 3.5f,
			"numeric ACE parameters preserve the float/int selector and float result");

		constexpr std::int32_t packed_position{static_cast<std::int32_t>(((static_cast<std::uint32_t>(-200) & 0xffffu) << 16) | 345u)};
		ipp_android::bridge::action(extension, 19, std::vector<InputValue>{
			string("Game"), string("Spawn"), integer(packed_position)});
		auto const x{ipp_android::bridge::expression(extension, 10,
			std::vector<InputValue>{string("Game"), string("Spawn")})};
		auto const y{ipp_android::bridge::expression(extension, 11,
			std::vector<InputValue>{string("Game"), string("Spawn")})};
		check(x.kind == ExpressionResult::Kind::Integer && x.integer == -200
			&& y.kind == ExpressionResult::Kind::Integer && y.integer == 345,
			"PARAM_POSITION keeps the packed high-word X and low-word Y encoding");

		ipp_android::bridge::action(extension, 4,
			std::vector<InputValue>{InputValue::object(), number(1), number(1)});
		check(!ipp_android::bridge::condition(extension, 4,
			std::vector<InputValue>{string("Game"), string("val00")}),
			"unmarshalled object-property ACE is guarded as a no-op instead of dereferencing a placeholder");

		static_cast<void>(ipp_android::bridge::handle(extension));
		auto const saved{sandbox / "nested" / "game.ini"};
		std::ifstream file{saved, std::ios::binary};
		std::string contents{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
		check(file.good() || file.eof(), "autosave opens the configured INI inside filesDir");
		check(contents.find("Hello, café 🐟") != std::string::npos && contents.find("Score=3.5") != std::string::npos,
			"autosave preserves UTF-8 text and numeric INI data");
		ipp_android::bridge::destroy(extension, true);
	}

	void run_legacy_edit_data_test(std::filesystem::path const& sandbox)
	{
		std::filesystem::create_directories(sandbox);
		// Version 1 strings are fixed-size Windows-1252 arrays. This exercises the café byte 0xE9.
		auto payload{make_v1_payload("legacy.ini", "[Legacy]\r\nText=caf\xE9\r\n")};
		auto* const extension{ipp_android::bridge::create(payload, 1, sandbox)};
		check(extension != nullptr, "create a runtime object from the legacy v1 ANSI edit-data payload");
		if(extension)
		{
			auto const value{ipp_android::bridge::expression(extension, 9,
				std::vector<InputValue>{string("Legacy"), string("Text"), string("")})};
			check(value.kind == ExpressionResult::Kind::String && value.text == "café",
				"v1 Windows-1252 edit strings decode to the same UTF-8 INI value");
			ipp_android::bridge::destroy(extension, true);
		}
	}

	void run_sandbox_tests(std::filesystem::path const& sandbox, std::filesystem::path const& parent)
	{
		std::filesystem::create_directories(sandbox);
		ipp_android::set_data_dir(sandbox);
		auto const direct_windows_path{ipp_android::safe_data_path(std::filesystem::path{"C:\\Sonic\\save.ini"})};
		check(direct_windows_path && *direct_windows_path == sandbox / "Sonic" / "save.ini",
			"legacy Windows drive paths map into the app sandbox, not a C: device path");
		check(!ipp_android::safe_data_path(std::filesystem::path{"../outside.ini"}),
			"parent-directory traversal is refused");
		check(!ipp_android::safe_data_path(std::filesystem::path{parent / "outside.ini"}),
			"absolute paths outside filesDir are refused");

		std::filesystem::create_directories(parent / "outside");
		std::error_code ec{};
		std::filesystem::create_directory_symlink(parent / "outside", sandbox / "escape-link", ec);
		if(!ec)
		{
			check(!ipp_android::safe_data_path(std::filesystem::path{"escape-link/secret.ini"}),
				"symlinks that resolve outside filesDir are refused");
		}
		else
		{
			check(true, "symlink escape test skipped because host filesystem disallows test symlinks");
		}
	}

	void run_file_lifecycle_tests(std::filesystem::path const& sandbox, std::filesystem::path const& parent)
	{
		// Fusion edit data may contain a Windows path. It is normalized beneath filesDir before
		// load/save, preserving the useful relative path without attempting to access a drive.
		auto drive_payload{make_v2_payload("C:\\Sonic\\Game.ini")};
		auto* const drive_extension{ipp_android::bridge::create(drive_payload, 2, sandbox)};
		check(drive_extension != nullptr, "create object with a legacy Windows-form default filename");
		if(drive_extension)
		{
			ipp_android::bridge::action(drive_extension, 15,
				std::vector<InputValue>{string("Game"), string("Loaded"), string("sandboxed")});
			static_cast<void>(ipp_android::bridge::handle(drive_extension));
			check(std::filesystem::exists(sandbox / "Sonic" / "Game.ini"),
				"legacy default filename autosaves under Context.getFilesDir()");
			ipp_android::bridge::destroy(drive_extension, true);
		}

		auto unsafe_payload{make_v2_payload("../escape.ini")};
		auto* const unsafe_extension{ipp_android::bridge::create(unsafe_payload, 2, sandbox)};
		check(unsafe_extension != nullptr, "unsafe paths do not prevent the INI object itself from loading");
		if(unsafe_extension)
		{
			ipp_android::bridge::action(unsafe_extension, 15,
				std::vector<InputValue>{string("Game"), string("Loaded"), string("not written outside")});
			static_cast<void>(ipp_android::bridge::handle(unsafe_extension));
			check(!std::filesystem::exists(parent / "escape.ini"),
				"outside-sandbox autosave is rejected before directory creation or file I/O");
			ipp_android::bridge::destroy(unsafe_extension, true);
		}
	}
}

int main()
{
	std::error_code ec{};
	auto const base{std::filesystem::temp_directory_path() / "iniplusplus-android-bridge-tests"};
	std::filesystem::remove_all(base, ec);
	auto const sandbox{base / "files"};
	std::filesystem::create_directories(base);

	run_sandbox_tests(sandbox, base);
	run_runtime_tests(sandbox);
	run_legacy_edit_data_test(sandbox);
	run_file_lifecycle_tests(sandbox, base);

	std::filesystem::remove_all(base, ec);
	std::cout << (failures == 0 ? "All Android bridge host tests passed.\n" : "Android bridge host tests failed.\n");
	return failures == 0 ? 0 : 1;
}
