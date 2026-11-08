#include "../AssArraySC2Store.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition)
        {
            throw std::runtime_error{std::string{message}};
        }
    }

    std::filesystem::path make_test_directory()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto base = std::filesystem::temp_directory_path() /
            ("assarraysc2-store-tests-" + std::to_string(stamp));
        std::filesystem::create_directories(base);
        return base;
    }

    void write_bytes(const std::filesystem::path& path, std::string_view bytes)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        require(static_cast<bool>(output), "write fixture bytes");
    }

    std::string read_bytes(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        return std::string{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: AssArraySC2StoreTests <ringdata.sav>\n";
        return 2;
    }

    const std::filesystem::path sample = std::filesystem::u8path(argv[1]);
    require(std::filesystem::is_regular_file(sample), "Sonic Classic 1 ringdata.sav fixture exists");
    const std::filesystem::path directory = make_test_directory();
    try
    {
        const auto copied_sample = directory / "ringdata.sav";
        std::filesystem::copy_file(sample, copied_sample);

        AssArraySC2Store store;
        require(store.load(copied_sample.wstring()), "load flat numeric INI format fixture");
        require(store.get(L"RING1392") == 1.0, "key lookup is ASCII case-insensitive");
        require(store.get(L"ring4533") == 1.0, "load the final sample key");
        require(store.get(L"ring999999") == 0.0, "missing values default to zero");

        store.set(L"RING9999", 1.0);
        store.set(L"score", 12.5);
        store.set(L"caf\u00e9", 3.25);
        store.set(L"invalid=key", 99.0);
        store.set(L"not-finite", std::numeric_limits<double>::infinity());
        require(store.get(L"ring9999") == 1.0, "set and get normalize key case");
        require(store.size() == 24, "store contains sample entries and valid additions only");

        const auto saved = directory / "roundtrip.sav";
        require(store.save(saved.wstring()), "save a round-trip flat INI file");
        const std::string saved_text = read_bytes(saved);
        require(saved_text.find('[') == std::string::npos, "save output has no section header");
        require(saved_text.find("ring9999=1\r\n") != std::string::npos,
            "save output uses lowercase keys and integer formatting");
        require(saved_text.find("score=12.5\r\n") != std::string::npos,
            "save output preserves decimal numeric values");
        require(saved_text.find("caf\xc3\xa9=3.25\r\n") != std::string::npos,
            "save output encodes keys as UTF-8");

        AssArraySC2Store reloaded;
        require(reloaded.load(saved.wstring()), "load the saved round-trip file");
        require(reloaded.get(L"RING9999") == 1.0, "round-trip retains numeric values");
        require(reloaded.get(L"score") == 12.5, "round-trip retains fractional values");
        require(reloaded.get(L"caf\u00e9") == 3.25, "round-trip retains UTF-8 keys");

        const auto sections = directory / "sections.ini";
        write_bytes(sections, "[save]\r\nLives = 4\r\n; comment\r\n\r\nRings=15\r\n");
        require(reloaded.load(sections.wstring()), "load supported INI sections and comments");
        require(reloaded.size() == 2, "sectioned file replaces the store contents");
        require(reloaded.get(L"lives") == 4.0 && reloaded.get(L"RINGS") == 15.0,
            "section names are ignored and keys stay case-insensitive");

        const auto opaque = directory / "opaque.sav";
        write_bytes(opaque, std::string_view{"\x16\xf4\x80\xb3\0\xd5", 6});
        require(!reloaded.load(opaque.wstring()), "reject binary/opaque data");
        require(reloaded.get(L"lives") == 4.0, "rejected binary data does not clear current values");

        const auto string_only = directory / "strings.ini";
        write_bytes(string_only, "[profile]\r\nname=player\r\n");
        require(!reloaded.load(string_only.wstring()), "reject a string-only file as a numeric array");
        require(reloaded.get(L"rings") == 15.0, "rejected string-only data leaves current values intact");

        const auto empty = directory / "empty.ini";
        write_bytes(empty, "; empty numeric file\r\n");
        require(reloaded.load(empty.wstring()), "load a valid empty/comment-only file");
        require(reloaded.size() == 0, "valid empty file clears the array");
        require(!reloaded.save(L""), "reject an empty save path");
        require(!reloaded.load(L""), "reject an empty load path");

        std::filesystem::remove_all(directory);
        std::cout << "AssArraySC2 store tests passed against the Sonic Classic 1 format fixture (not SC2 compatibility evidence).\n";
        return 0;
    }
    catch (...)
    {
        std::filesystem::remove_all(directory);
        throw;
    }
}
