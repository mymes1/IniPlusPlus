#include "FusionAPI.hpp"
#include "lSDK/FusionUtilities.hpp"
#include "RunData.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace
{
    [[nodiscard]] std::wstring_view as_wide_view(lSDK::string_view_t value) noexcept
    {
        return value.data() ? std::wstring_view{value.data(), value.size()} : std::wstring_view{};
    }

    [[nodiscard]] std::int32_t float_result(float value) noexcept
    {
        std::int32_t result = 0;
        static_assert(sizeof(result) == sizeof(value));
        std::memcpy(&result, &value, sizeof(result));
        return result;
    }
}

#ifdef FUSION_ACTIONS
auto FUSION_API action_set_value_at_key(RunData* const run_data,
    std::int32_t const param0, std::int32_t const param1) noexcept -> std::int16_t
{
    try
    {
        lSDK::FusionParams<lSDK::FusionAceType::Action, 2> params{run_data->rHo, param0, param1};
        const lSDK::string_view_t key = params.next_string();
        const double value = params.next_float();
        if (run_data->store)
        {
            run_data->store->set(as_wide_view(key), value);
        }
    }
    catch (...)
    {
        // Extension actions have no error return channel in this compatibility subset.
    }
    return 0;
}

auto FUSION_API action_clear_array(RunData* const run_data,
    std::int32_t const, std::int32_t const) noexcept -> std::int16_t
{
    if (run_data->store)
    {
        run_data->store->clear();
    }
    return 0;
}

auto FUSION_API action_load_ini_file_into_values(RunData* const run_data,
    std::int32_t const param0, std::int32_t const param1) noexcept -> std::int16_t
{
    try
    {
        lSDK::FusionParams<lSDK::FusionAceType::Action, 1> params{run_data->rHo, param0, param1};
        const lSDK::string_view_t path = params.next_string();
        if (run_data->store)
        {
            (void)run_data->store->load(as_wide_view(path));
        }
    }
    catch (...)
    {
    }
    return 0;
}

auto FUSION_API action_save_ini_file_from_values(RunData* const run_data,
    std::int32_t const param0, std::int32_t const param1) noexcept -> std::int16_t
{
    try
    {
        lSDK::FusionParams<lSDK::FusionAceType::Action, 1> params{run_data->rHo, param0, param1};
        const lSDK::string_view_t path = params.next_string();
        if (run_data->store)
        {
            (void)run_data->store->save(as_wide_view(path));
        }
    }
    catch (...)
    {
    }
    return 0;
}

auto FUSION_API action_unused(RunData* const, std::int32_t const, std::int32_t const) noexcept
    -> std::int16_t
{
    return 0;
}
#endif

#ifdef FUSION_CONDITIONS
auto FUSION_API condition_unused(RunData* const, std::int32_t const, std::int32_t const) noexcept
    -> std::int32_t
{
    return FUSION_CONDITIONS_FALSE;
}
#endif

#ifdef FUSION_EXPRESSIONS
auto FUSION_API expression_get_value(RunData* const run_data, std::int32_t const params_handle) noexcept
    -> std::int32_t
{
    try
    {
        lSDK::FusionParams<lSDK::FusionAceType::Expression, 1> params{run_data->rHo, params_handle};
        const lSDK::string_view_t key = params.next_string();
        const double value = run_data->store ? run_data->store->get(as_wide_view(key)) : 0.0;
        const float result = static_cast<float>(value);
        run_data->rHo.hoFlags |= HOF_FLOAT;
        return float_result(std::isfinite(result) ? result : 0.0F);
    }
    catch (...)
    {
        run_data->rHo.hoFlags |= HOF_FLOAT;
        return float_result(0.0F);
    }
}
#endif
