#include "AssArraySC2Store.hpp"

#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace
{
    constexpr std::size_t maximum_file_bytes = 16U * 1024U * 1024U;

    [[nodiscard]] bool is_space(wchar_t value) noexcept
    {
        return std::iswspace(static_cast<wint_t>(value)) != 0;
    }

    [[nodiscard]] std::wstring_view trim(std::wstring_view value) noexcept
    {
        while (!value.empty() && is_space(value.front()))
        {
            value.remove_prefix(1);
        }
        while (!value.empty() && is_space(value.back()))
        {
            value.remove_suffix(1);
        }
        return value;
    }

    [[nodiscard]] bool normalize_key(std::wstring_view key, std::wstring& normalized)
    {
        key = trim(key);
        if (key.empty())
        {
            return false;
        }

        normalized.clear();
        normalized.reserve(key.size());
        for (wchar_t value : key)
        {
            if (value == L'=' || value == L'\r' || value == L'\n' || value == L'\0')
            {
                normalized.clear();
                return false;
            }
            if (value >= L'A' && value <= L'Z')
            {
                value = static_cast<wchar_t>(value - L'A' + L'a');
            }
            normalized.push_back(value);
        }
        return true;
    }

    [[nodiscard]] bool append_code_point(std::wstring& output, std::uint32_t code_point)
    {
        if (code_point > 0x10FFFFU || (code_point >= 0xD800U && code_point <= 0xDFFFU))
        {
            return false;
        }
        if constexpr (sizeof(wchar_t) == 2)
        {
            if (code_point <= 0xFFFFU)
            {
                output.push_back(static_cast<wchar_t>(code_point));
            }
            else
            {
                code_point -= 0x10000U;
                output.push_back(static_cast<wchar_t>(0xD800U + (code_point >> 10U)));
                output.push_back(static_cast<wchar_t>(0xDC00U + (code_point & 0x3FFU)));
            }
        }
        else
        {
            output.push_back(static_cast<wchar_t>(code_point));
        }
        return true;
    }

    [[nodiscard]] bool decode_utf8(std::string_view input, std::wstring& output)
    {
        output.clear();
        output.reserve(input.size());
        for (std::size_t index = 0; index < input.size();)
        {
            const auto first = static_cast<unsigned char>(input[index]);
            std::uint32_t code_point = 0;
            std::size_t trailing = 0;
            if (first <= 0x7FU)
            {
                code_point = first;
            }
            else if (first >= 0xC2U && first <= 0xDFU)
            {
                code_point = first & 0x1FU;
                trailing = 1;
            }
            else if (first >= 0xE0U && first <= 0xEFU)
            {
                code_point = first & 0x0FU;
                trailing = 2;
            }
            else if (first >= 0xF0U && first <= 0xF4U)
            {
                code_point = first & 0x07U;
                trailing = 3;
            }
            else
            {
                return false;
            }

            if (index + trailing >= input.size())
            {
                return false;
            }
            for (std::size_t offset = 1; offset <= trailing; ++offset)
            {
                const auto next = static_cast<unsigned char>(input[index + offset]);
                if ((next & 0xC0U) != 0x80U)
                {
                    return false;
                }
                code_point = (code_point << 6U) | (next & 0x3FU);
            }
            if ((trailing == 1 && code_point < 0x80U)
                || (trailing == 2 && code_point < 0x800U)
                || (trailing == 3 && code_point < 0x10000U)
                || !append_code_point(output, code_point))
            {
                return false;
            }
            index += trailing + 1;
        }
        return true;
    }

    [[nodiscard]] bool append_utf8(std::string& output, std::uint32_t code_point)
    {
        if (code_point > 0x10FFFFU || (code_point >= 0xD800U && code_point <= 0xDFFFU))
        {
            return false;
        }
        if (code_point <= 0x7FU)
        {
            output.push_back(static_cast<char>(code_point));
        }
        else if (code_point <= 0x7FFU)
        {
            output.push_back(static_cast<char>(0xC0U | (code_point >> 6U)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
        }
        else if (code_point <= 0xFFFFU)
        {
            output.push_back(static_cast<char>(0xE0U | (code_point >> 12U)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
        }
        else
        {
            output.push_back(static_cast<char>(0xF0U | (code_point >> 18U)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 12U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
        }
        return true;
    }

    [[nodiscard]] bool encode_utf8(std::wstring_view input, std::string& output)
    {
        output.clear();
        output.reserve(input.size());
        for (std::size_t index = 0; index < input.size(); ++index)
        {
            std::uint32_t code_point = static_cast<std::uint32_t>(input[index]);
            if constexpr (sizeof(wchar_t) == 2)
            {
                if (code_point >= 0xD800U && code_point <= 0xDBFFU)
                {
                    if (++index >= input.size())
                    {
                        return false;
                    }
                    const std::uint32_t low = static_cast<std::uint32_t>(input[index]);
                    if (low < 0xDC00U || low > 0xDFFFU)
                    {
                        return false;
                    }
                    code_point = 0x10000U + ((code_point - 0xD800U) << 10U) + (low - 0xDC00U);
                }
                else if (code_point >= 0xDC00U && code_point <= 0xDFFFU)
                {
                    return false;
                }
            }
            if (!append_utf8(output, code_point))
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool read_file(const std::filesystem::path& path, std::string& bytes)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            return false;
        }

        input.seekg(0, std::ios::end);
        const std::streamoff length = input.tellg();
        if (length < 0 || static_cast<std::uint64_t>(length) > maximum_file_bytes)
        {
            return false;
        }
        input.seekg(0, std::ios::beg);
        bytes.resize(static_cast<std::size_t>(length));
        if (!bytes.empty())
        {
            input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            if (!input)
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool parse_number(std::wstring_view text, double& value)
    {
        text = trim(text);
        if (text.empty())
        {
            return false;
        }
        const std::wstring buffer{text};
        wchar_t* end = nullptr;
        errno = 0;
        const double parsed = std::wcstod(buffer.c_str(), &end);
        if (end == buffer.c_str() || *end != L'\0' || errno == ERANGE || !std::isfinite(parsed))
        {
            return false;
        }
        value = parsed;
        return true;
    }

    [[nodiscard]] std::string format_number(double value)
    {
        std::array<char, 64> buffer{};
        const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::general);
        if (result.ec != std::errc{})
        {
            throw std::runtime_error{"Could not format AssArraySC2 value"};
        }
        return std::string{buffer.data(), result.ptr};
    }

    [[nodiscard]] bool is_section_header(std::wstring_view line) noexcept
    {
        line = trim(line);
        return line.size() >= 2 && line.front() == L'[' && line.back() == L']';
    }
}

void AssArraySC2Store::clear() noexcept
{
    values_.clear();
}

void AssArraySC2Store::set(std::wstring_view key, double value)
{
    if (!std::isfinite(value))
    {
        return;
    }
    std::wstring normalized;
    if (normalize_key(key, normalized))
    {
        values_[std::move(normalized)] = value;
    }
}

double AssArraySC2Store::get(std::wstring_view key) const noexcept
{
    try
    {
        std::wstring normalized;
        if (!normalize_key(key, normalized))
        {
            return 0.0;
        }
        const auto found = values_.find(normalized);
        return found == values_.end() ? 0.0 : found->second;
    }
    catch (...)
    {
        return 0.0;
    }
}

bool AssArraySC2Store::load(std::wstring_view requested_path) noexcept
{
    try
    {
        if (requested_path.empty() || requested_path.find(L'\0') != std::wstring_view::npos)
        {
            return false;
        }

        const std::filesystem::path path{std::wstring{requested_path}};
        std::string bytes;
        if (!read_file(path, bytes))
        {
            return false;
        }
        if (bytes.find('\0') != std::string::npos)
        {
            return false;
        }
        if (bytes.size() >= 3
            && static_cast<unsigned char>(bytes[0]) == 0xEFU
            && static_cast<unsigned char>(bytes[1]) == 0xBBU
            && static_cast<unsigned char>(bytes[2]) == 0xBFU)
        {
            bytes.erase(0, 3);
        }

        std::wstring text;
        if (!decode_utf8(bytes, text))
        {
            return false;
        }

        Values loaded;
        bool saw_assignment = false;
        bool saw_other_content = false;
        std::size_t line_start = 0;
        while (line_start <= text.size())
        {
            std::size_t line_end = text.find_first_of(L"\r\n", line_start);
            if (line_end == std::wstring::npos)
            {
                line_end = text.size();
            }
            const std::wstring_view line{text.data() + line_start, line_end - line_start};
            const std::wstring_view trimmed = trim(line);
            if (!trimmed.empty() && trimmed.front() != L';' && trimmed.front() != L'#')
            {
                if (is_section_header(trimmed))
                {
                    // The SC2 compatibility subset stores one flat key namespace. Section names
                    // are ignored rather than being folded into keys.
                }
                else
                {
                    const std::size_t equals = trimmed.find(L'=');
                    if (equals == std::wstring_view::npos)
                    {
                        saw_other_content = true;
                    }
                    else
                    {
                        saw_assignment = true;
                        std::wstring key;
                        if (normalize_key(trimmed.substr(0, equals), key))
                        {
                            double number = 0.0;
                            if (parse_number(trimmed.substr(equals + 1), number))
                            {
                                loaded[std::move(key)] = number;
                            }
                        }
                    }
                }
            }

            if (line_end == text.size())
            {
                break;
            }
            line_start = line_end + 1;
            if (text[line_end] == L'\r' && line_start < text.size() && text[line_start] == L'\n')
            {
                ++line_start;
            }
        }

        // A string-only or opaque INI must not be mistaken for an empty numeric array and erase
        // the current data. A genuinely empty/comment-only file remains a valid empty array.
        if (loaded.empty() && (saw_assignment || saw_other_content))
        {
            return false;
        }
        values_.swap(loaded);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool AssArraySC2Store::save(std::wstring_view requested_path) const noexcept
{
    try
    {
        if (requested_path.empty() || requested_path.find(L'\0') != std::wstring_view::npos)
        {
            return false;
        }

        const std::filesystem::path path{std::wstring{requested_path}};
        const std::filesystem::path parent = path.parent_path();
        if (!parent.empty())
        {
            std::error_code error;
            std::filesystem::create_directories(parent, error);
            if (error && !std::filesystem::is_directory(parent))
            {
                return false;
            }
        }

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            return false;
        }
        for (const auto& [key, value] : values_)
        {
            std::string key_bytes;
            if (!encode_utf8(key, key_bytes))
            {
                return false;
            }
            const std::string number = format_number(value);
            output.write(key_bytes.data(), static_cast<std::streamsize>(key_bytes.size()));
            output.put('=');
            output.write(number.data(), static_cast<std::streamsize>(number.size()));
            output.write("\r\n", 2);
            if (!output)
            {
                return false;
            }
        }
        output.flush();
        return static_cast<bool>(output);
    }
    catch (...)
    {
        return false;
    }
}

std::size_t AssArraySC2Store::size() const noexcept
{
    return values_.size();
}
