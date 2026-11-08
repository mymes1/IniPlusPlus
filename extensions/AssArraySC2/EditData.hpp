#pragma once

#include "FusionAPI.hpp"

struct SerializedEditData final
{
    static constexpr std::int16_t CURRENT_VERSION = 0;
    extHeader eHeader;

    SerializedEditData() = delete;
    SerializedEditData(SerializedEditData const&) = delete;
    SerializedEditData(SerializedEditData&&) = delete;
    SerializedEditData& operator=(SerializedEditData const&) = delete;
    SerializedEditData& operator=(SerializedEditData&&) = delete;
    ~SerializedEditData() = delete;
};

static_assert(fusion::is_valid_editdata_structure());
