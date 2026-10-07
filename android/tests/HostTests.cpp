// Ini++ Android runtime: host test harness.
//
// Compiles the *Android* implementation (Runtime.cpp, ACEs.cpp, android/runtime/AndroidRuntime.cpp
// and the exported entry points of android/jni/IniPlusPlusExtension.cc) with a host compiler and
// drives it through a mock of the Fusion Android native extension API.  This is the same code the
// Android .so is built from; only the RuntimeFunctions implementation and the edit data blob are
// provided by the test, so a green run means:
//
//   * the object can be created from edit data written the way the Windows editor writes it
//   * groups, items, numbers and strings can be set and read back through the ACEs
//   * Save / Load / SaveAs write and read the INI through the Android platform file layer
//   * autosave and the "current group" behaviour work
//   * conditions and expressions return the same values as the Windows implementation
//
// Build and run: tools/run_host_tests.sh

#include "FusionAPI.hpp"
#include "lSDK.hpp"

#include <RuntimeNative.h>

#include "AceDeclarations.inc" // generated ACE counts, for the assertions below

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------------------------------------------------
// The exported entry points of the Android extension (see android/jni/IniPlusPlusExtension.cc)
// -------------------------------------------------------------------------------------------------
struct Extension;
// The extension's exported entry points (CRunIniPlusPlus_* in android/jni/IniPlusPlusExtension.cc,
// the plain-identifier spelling of the official Android extension ABI).
extern "C"
{
	void CRunIniPlusPlus_extInit();
	void* CRunIniPlusPlus_createRunObject(void* edPtr, void* ext, RuntimeFunctions* fn);
	int CRunIniPlusPlus_getNumberOfConditions();
	void CRunIniPlusPlus_destroyRunObject(Extension* e);
	int CRunIniPlusPlus_handleRunObject(Extension* e);
	void CRunIniPlusPlus_action(Extension* e, int num, void* act);
	bool CRunIniPlusPlus_condition(Extension* e, int num, void* cnd);
	void CRunIniPlusPlus_expression(Extension* e, int num, void* exp);
}

namespace
{
	int failures = 0;
	void check(bool const ok, std::string_view const what)
	{
		std::printf("%s %.*s\n", ok ? "  ok  " : "FAIL  ", static_cast<int>(what.size()), what.data());
		if(!ok)
		{
			++failures;
		}
	}

	// ---- mock Fusion Android runtime -----------------------------------------------------------------
	struct MockAce
	{
		// parameters declared for the ACE being called, in order, as strings (Fusion expressions
		// are evaluated to strings/numbers by the runtime; the mock keeps both forms)
		std::vector<std::string> strings;
		std::vector<float> numbers;
	};

	struct MockRuntime final
	{
		MockAce ace;
		std::size_t string_index{};
		std::size_t number_index{};
		float last_return_float{};
		std::int32_t last_return_int{};
		std::string last_return_string;
		bool return_is_float{};

		void begin(std::vector<std::string> strings, std::vector<float> numbers)
		{
			ace.strings = std::move(strings);
			ace.numbers = std::move(numbers);
			string_index = number_index = 0;
		}

		RuntimeFunctions::string take_string()
		{
			auto const& s{ace.strings.at(string_index++)};
			return RuntimeFunctions::string{const_cast<char*>(s.c_str()), static_cast<std::int32_t>(s.size()), 0};
		}
		float take_number() { return ace.numbers.at(number_index++); }
	};

	MockRuntime mock;

	RuntimeFunctions const make_runtime_functions()
	{
		RuntimeFunctions rt{};
		rt.freeString = [](void*, RuntimeFunctions::string) {};
		rt.act_getParamExpression = [](void*, void*) { return static_cast<int>(mock.take_number()); };
		rt.act_getParamExpString = [](void*, void*) { return mock.take_string(); };
		rt.act_getParamExpFloat = [](void*, void*) { return mock.take_number(); };
		rt.cnd_getParamExpression = [](void*, void*) { return static_cast<int>(mock.take_number()); };
		rt.cnd_getParamExpString = [](void*, void*) { return mock.take_string(); };
		rt.cnd_getParamExpFloat = [](void*, void*) { return mock.take_number(); };
		rt.exp_getParamInt = [](void*, void*) { return static_cast<int>(mock.take_number()); };
		rt.exp_getParamString = [](void*, void*) { return mock.take_string(); };
		rt.exp_getParamFloat = [](void*, void*) { return mock.take_number(); };
		rt.exp_setReturnInt = [](void*, void*, int value) { mock.last_return_int = value; mock.return_is_float = false; };
		rt.exp_setReturnString = [](void*, void*, char const* value) { mock.last_return_string = value ? value : ""; };
		rt.exp_setReturnFloat = [](void*, void*, float value) { mock.last_return_float = value; mock.return_is_float = true; };
		rt.generateEvent = [](void*, int, int) {};
		return rt;
	}

	// ---- edit data, in the byte layout Edittime.cpp::serialize() writes -----------------------------
	std::vector<std::byte> make_editdata(std::string const& default_text, std::string const& default_file,
	                                     bool const auto_save, bool const read_only, std::string const& update,
	                                     std::string const& line_ending, bool const std_ini,
	                                     std::int32_t const repeat_groups, bool const global_object,
	                                     std::string const& global_key)
	{
		std::vector<std::byte> raw;
		auto const put_u8 = [&raw](std::uint8_t const v) { raw.push_back(static_cast<std::byte>(v)); };
		auto const put_i8 = [&raw](std::int8_t const v) { raw.push_back(static_cast<std::byte>(v)); };
		auto const put_i32 = [&raw](std::int32_t const v)
		{
			for(std::size_t i{}; i < 4; ++i)
			{
				raw.push_back(static_cast<std::byte>((static_cast<std::uint32_t>(v) >> (8 * i)) & 0xFF));
			}
		};
		// version 2 strings are UTF-16LE and NUL terminated (TCHAR is 2 bytes on Windows)
		auto const put_str = [&raw](std::string const& s)
		{
			for(auto const c : s)
			{
				raw.push_back(static_cast<std::byte>(c));
				raw.push_back(std::byte{});
			}
			raw.push_back(std::byte{});
			raw.push_back(std::byte{});
		};
		put_u8(0);            // b_defaultFile
		put_u8(read_only ? 1 : 0);
		put_str(default_file);
		put_i32(0);           // defaultFolder
		put_str(default_text);
		put_u8(1);            // bool_CanCreateFolders
		put_u8(auto_save ? 1 : 0);
		put_u8(std_ini ? 1 : 0);
		put_u8(0);            // bool_compress
		put_u8(0);            // bool_encrypt
		put_str("");          // encrypt_key
		put_u8(0);            // bool_newline
		put_str(line_ending); // newline
		put_u8(1);            // bool_QuoteStrings
		put_i32(repeat_groups);
		put_i8(0);            // repeatItems
		put_i8(10);           // undoCount
		put_i8(10);           // redoCount
		put_u8(0);            // saveRepeated
		put_u8(0);            // bool_EscapeGroup
		put_u8(0);            // bool_EscapeItem
		put_u8(0);            // bool_EscapeValue
		put_u8(0);            // bool_CaseSensitive
		put_u8(global_object ? 1 : 0);
		put_u8(0);            // index
		put_u8(0);            // autoLoad
		put_u8(0);            // subGroups
		put_u8(0);            // allowEmptyGroups
		put_str(global_key);

		// eHeader (extHeader) + payload, as Edittime.cpp's serialize() lays it out through
		// mvReAllocEditData(): five 32-bit fields, then the serialized properties.
		std::vector<std::byte> blob;
		auto const put_u32 = [&blob](std::uint32_t const v)
		{
			for(std::size_t i{}; i < 4; ++i)
			{
				blob.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF));
			}
		};
		put_u32(static_cast<std::uint32_t>(20 + raw.size())); // extSize
		put_u32(static_cast<std::uint32_t>(20 + raw.size())); // extMaxSize
		put_u32(2);                                           // extVersion = INIPP_V2_UNICODE
		put_u32(0);                                           // extID
		put_u32(0);                                           // extPrivateData
		blob.insert(blob.end(), raw.begin(), raw.end());
		return blob;
	}

	void run_action(Extension* const e, int const num, std::vector<std::string> strings = {}, std::vector<float> numbers = {})
	{
		mock.begin(std::move(strings), std::move(numbers));
		CRunIniPlusPlus_action(e, num, &mock.ace);
	}
	float run_expression_float(Extension* const e, int const num, std::vector<std::string> strings = {}, std::vector<float> numbers = {})
	{
		mock.begin(std::move(strings), std::move(numbers));
		mock.last_return_float = 0;
		mock.last_return_int = 0;
		mock.last_return_string.clear();
		CRunIniPlusPlus_expression(e, num, &mock.ace);
		// An expression either sets HOF_FLOAT (bit pattern) or returns a plain integer.
		return mock.return_is_float ? mock.last_return_float : static_cast<float>(mock.last_return_int);
	}
	std::string run_expression_string(Extension* const e, int const num, std::vector<std::string> strings = {}, std::vector<float> numbers = {})
	{
		mock.begin(std::move(strings), std::move(numbers));
		mock.last_return_float = 0;
		mock.last_return_int = 0;
		mock.last_return_string.clear();
		CRunIniPlusPlus_expression(e, num, &mock.ace);
		return mock.last_return_string;
	}
	bool run_condition(Extension* const e, int const num, std::vector<std::string> strings = {}, std::vector<float> numbers = {})
	{
		mock.begin(std::move(strings), std::move(numbers));
		return CRunIniPlusPlus_condition(e, num, &mock.ace);
	}

	// ACE ids: the tables at the bottom of ACEs.cpp (and the declarations in Menus.cpp)
	constexpr int Action_SetCurrentGroup{0};
	constexpr int Action_SetValueG{1};   // "Set value" (group parameter)
	constexpr int Action_SetStringG{2};
	constexpr int Action_SetValue{14};   // "Set value" (current group)
	constexpr int Action_SetString{15};
	constexpr int Action_SaveObject{17}; // object based: no object data on Android, must be a no-op
	constexpr int Action_New{42};
	constexpr int Action_Load{43};
	constexpr int Action_Save{44};
	constexpr int Action_SaveAs{45};
	constexpr int Condition_HasGroup{3};
	constexpr int Condition_HasItem{4};
	constexpr int Expression_GetItemValueG{0};
	constexpr int Expression_GetItemStringG{1};
	constexpr int Expression_ItemCountG{7};
	constexpr int Expression_GetItemValue{8};
	constexpr int Expression_GetItemString{9};
	constexpr int Expression_GroupCount{16};
	constexpr int Expression_ItemCount{17};
	constexpr int Expression_TotalItems{18};
	constexpr int Expression_Fname{40};
}

int main()
{
	std::setvbuf(stdout, nullptr, _IONBF, 0); // unbuffered: CI logs must survive a crash
	auto const dir{std::filesystem::temp_directory_path() / "inipp-android-host-tests"};
	std::error_code ec{};
	std::filesystem::remove_all(dir, ec);
	std::filesystem::create_directories(dir, ec);
	::setenv("INIPLUSPLUS_ANDROID_DATA_DIR", dir.string().c_str(), 1);

	RuntimeFunctions const rt{make_runtime_functions()};
	CRunIniPlusPlus_extInit();
	check(CRunIniPlusPlus_getNumberOfConditions() == ipp_android::number_of_conditions, "getNumberOfConditions() matches the generated ACE table (Menus.cpp)");

	// ---- create an object with default text, in the Windows editor's byte layout ----
	auto const edPtr{make_editdata("[Group1]\nItem1=42\nItem2=hello\n", "", true /*autosave*/, false, "", "\r\n", true, 0, false, "")};
	auto* const e{static_cast<Extension*>(CRunIniPlusPlus_createRunObject(const_cast<std::vector<std::byte>*>(&edPtr)->data(), nullptr, const_cast<RuntimeFunctions*>(&rt)))};
	check(e != nullptr, "createRunObject() succeeds with a Windows-format edit data blob");
	if(!e)
	{
		return 1;
	}

	// ---- conditions and expressions on the default text ----
	check(run_condition(e, Condition_HasGroup, {"Group1"}), "Has group: sees the group loaded from the default text");
	check(!run_condition(e, Condition_HasGroup, {"Group2"}), "Has group: missing group returns false");
	check(run_condition(e, Condition_HasItem, {"Group1", "Item1"}), "Has item: finds Item1");
	check(run_condition(e, Condition_HasItem, {"Group1", "Nope"}) == false, "Has item: missing item returns false");
	run_action(e, Action_SetCurrentGroup, {"Group1"}, {0});
	check(run_expression_float(e, Expression_GetItemValueG, {"Item1"}, {0}) == 42.0f, "Get value (current group) reads 42");
	check(run_expression_string(e, Expression_GetItemStringG, {"Item2", ""}) == "hello", "Get string (current group) reads hello");
	check(run_expression_float(e, Expression_GetItemValue, {"Group1", "Item1"}, {0}) == 42.0f, "Get value (group) reads 42");
	check(run_expression_float(e, Expression_GroupCount) == 1.0f, "Group count is 1");
	check(run_expression_float(e, Expression_ItemCountG) == 2.0f, "Item count in the current group is 2");
	check(run_expression_float(e, Expression_TotalItems) == 2.0f, "Total item count is 2");

	// ---- write values and strings, create new groups ----
	run_action(e, Action_SetCurrentGroup, {"NewGroup"}, {0});
	run_action(e, Action_SetValueG, {"Value1"}, {1, 1234});    // flag 1 = write a number
	run_action(e, Action_SetStringG, {"String1", "Android"});  // item, string value
	check(run_expression_float(e, Expression_GetItemValueG, {"Value1"}, {0}) == 1234.0f, "Set value: numeric write and read-back");
	check(run_expression_string(e, Expression_GetItemStringG, {"String1", ""}) == "Android", "Set string: write and read-back");
	check(run_expression_float(e, Expression_GroupCount) == 2.0f, "Creating a group increments the group count");
	check(run_condition(e, Condition_HasGroup, {"NewGroup"}), "Set current group created the new group on write");

	// ---- the "current group" ACE variant (hoEventNumber == 1) ----
	run_action(e, Action_SetValue, {"NewGroup", "Value2"}, {1, 99}); // ACE 14 takes the group first
	check(run_expression_float(e, Expression_GetItemValueG, {"Value2"}, {0}) == 99.0f, "Set value (current group): numeric write");

	// ---- ACEs that need another object: the Android API has no object references, so they must do
	// nothing instead of dereferencing the zeroed block (the object header's OEFLAGS are 0) ----
	run_action(e, Action_SaveObject, {"NewGroup"}, {1, 0});
	check(run_expression_float(e, Expression_GroupCount) == 2.0f, "Save object properties: safe no-op on Android");

	// ---- save to a file in the app sandbox, wipe, load it back ----
	auto const ini_path{dir / "saved.ini"};
	run_action(e, Action_SaveAs, {ini_path.string()});
	check(std::filesystem::exists(ini_path), "Save as: the INI file exists in the app data directory");
	{
		auto const text{[](std::filesystem::path const& p) {
			std::ifstream ifs{p, std::ios::binary};
			return std::string{std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{}};
		}(ini_path)};
		check(text.find("[NewGroup]") != std::string::npos, "Save as: group header written");
		check(text.find("Value1=1234") != std::string::npos, "Save as: numeric item written");
		check(text.find("String1=Android") != std::string::npos, "Save as: string item written");
	}
	run_action(e, Action_New, {ini_path.string()}, {0}); // New(file, keep autosave flag)
	check(run_expression_float(e, Expression_GroupCount) == 0.0f, "New: clears the object");
	check(run_expression_string(e, Expression_Fname, {}, {}) == ini_path.string(), "File name expression still reports the file");
	run_action(e, Action_Load, {ini_path.string()}, {0});
	check(run_expression_float(e, Expression_GetItemValue, {"NewGroup", "Value1"}, {0}) == 1234.0f, "Load: numeric value restored");
	check(run_expression_string(e, Expression_GetItemString, {"NewGroup", "String1", ""}) == "Android", "Load: string value restored");
	check(run_expression_float(e, Expression_GroupCount) == 2.0f, "Load: both groups restored");

	// ---- handleRunObject() drives autosave (the object was created with autosave enabled) ----
	run_action(e, Action_SetCurrentGroup, {"NewGroup"}, {0});
	run_action(e, Action_SetValueG, {"Value1"}, {1, 4321});
	CRunIniPlusPlus_handleRunObject(e);
	{
		auto const text{[](std::filesystem::path const& p) {
			std::ifstream ifs{p, std::ios::binary};
			return std::string{std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{}};
		}(ini_path)};
		check(text.find("Value1=4321") != std::string::npos, "handleRunObject(): autosaves changes to the open file");
	}

	// ---- files written by Windows tools: UTF-16 (BOM) and ANSI/Windows-1252 ----
	{
		std::string utf16{"\xFF\xFE"};                       // UTF-16LE BOM
		std::u16string const text{u"[WindowsGroup]\nItem1=7\nItem2=h\u00E9llo\n"};
		for(char16_t const unit : text)
		{
			utf16 += static_cast<char>(unit & 0xFF);
			utf16 += static_cast<char>((unit >> 8) & 0xFF);
		}
		auto const utf16_path{dir / "windows-utf16.ini"};
		{ std::ofstream ofs{utf16_path, std::ios::binary}; ofs.write(utf16.data(), static_cast<std::streamsize>(utf16.size())); }
		run_action(e, Action_Load, {utf16_path.string()}, {0});
		check(run_expression_float(e, Expression_GetItemValue, {"WindowsGroup", "Item1"}, {0}) == 7.0f,
		      "Load: a UTF-16 (BOM) INI written by a Windows tool reads correctly");
		check(run_expression_string(e, Expression_GetItemString, {"WindowsGroup", "Item2", ""}) == "h\xC3\xA9llo",
		      "Load: the UTF-16 text is converted to UTF-8");
	}
	{
		std::string ansi{"[AnsiGroup]\nItem1=3\nItem2=caf\xE9\n"}; // Windows-1252, no BOM
		auto const ansi_path{dir / "windows-ansi.ini"};
		{ std::ofstream ofs{ansi_path, std::ios::binary}; ofs.write(ansi.data(), static_cast<std::streamsize>(ansi.size())); }
		run_action(e, Action_Load, {ansi_path.string()}, {0});
		check(run_expression_float(e, Expression_GetItemValue, {"AnsiGroup", "Item1"}, {0}) == 3.0f,
		      "Load: an ANSI (Windows-1252) INI reads correctly");
		check(run_expression_string(e, Expression_GetItemString, {"AnsiGroup", "Item2", ""}) == "caf\xC3\xA9",
		      "Load: Windows-1252 text is converted to UTF-8");
	}

	CRunIniPlusPlus_destroyRunObject(e);
	std::printf("\n%s\n", failures == 0 ? "all Android runtime tests passed" : "SOME TESTS FAILED");
	return failures == 0 ? 0 : 1;
}
