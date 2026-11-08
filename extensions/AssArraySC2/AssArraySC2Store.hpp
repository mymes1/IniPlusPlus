#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>

// Independent numeric key/value store for the AssArraySC2 replacement extension.
class AssArraySC2Store final
{
public:
    void clear() noexcept;
    void set(std::wstring_view key, double value);
    [[nodiscard]] double get(std::wstring_view key) const noexcept;
    [[nodiscard]] bool load(std::wstring_view path) noexcept;
    [[nodiscard]] bool save(std::wstring_view path) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    using Values = std::map<std::wstring, double>;
    Values values_;
};
