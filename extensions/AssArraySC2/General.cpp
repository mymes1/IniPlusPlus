#include "EditData.hpp"
#include "FusionAPI.hpp"
#include "RunData.hpp"

#include <cstdint>
#include <iterator>

#ifdef FUSION_GET_COMPATIBILITY
auto FUSION_API GetInfos(std::int32_t const which) noexcept -> std::uint32_t
{
    switch (which)
    {
    case KGI_VERSION:
        return FUSION_GET_COMPATIBILITY_VERSION;
    case KGI_PLUGIN:
        return FUSION_GET_COMPATIBILITY_PLUGIN;
    case KGI_PRODUCT:
        return PRODUCT_VERSION_STANDARD;
    case KGI_BUILD:
        return 295;
    case KGI_UNICODE:
        return FUSION_GET_COMPATIBILITY_UNICODE;
    case KGI_MULTIPLESUBTYPE:
        return FUSION_GET_COMPATIBILITY_MULTIPLE_SUBTYPES;
    default:
        return 0;
    }
}
#endif

#ifdef FUSION_ACTIONS
fusion::action_func action_set_value_at_key;
fusion::action_func action_clear_array;
fusion::action_func action_load_ini_file_into_values;
fusion::action_func action_save_ini_file_from_values;
fusion::action_func action_unused;
#endif

#ifdef FUSION_CONDITIONS
fusion::condition_func condition_unused;
#endif

#ifdef FUSION_EXPRESSIONS
fusion::expression_func expression_get_value;
#endif

#ifdef FUSION_GET_EXTENSION_INFO
auto FUSION_API GetRunObjectInfos(mv* const, kpxRunInfos* const run_info) noexcept -> std::int16_t
{
    if (!run_info)
    {
        return FUSION_GET_EXTENSION_INFO_FAILURE;
    }

    run_info->identifier = static_cast<long>(
        static_cast<std::uint32_t>('A')
        | (static_cast<std::uint32_t>('S') << 8U)
        | (static_cast<std::uint32_t>('2') << 16U)
        | (static_cast<std::uint32_t>('C') << 24U)); // Unique FourCC: AS2C
    run_info->version = SerializedEditData::CURRENT_VERSION;
    run_info->editDataSize = sizeof(SerializedEditData);
    run_info->windowProcPriority = 100;
    run_info->editFlags = FUSION_OEFLAGS;
    run_info->editPrefs = 0;

    // Preserve original IDs 0 through 14 so the five SC2 actions dispatch at their legacy IDs.
    // Only IDs 0, 3, 12, and 14 are exposed in the editor menu; the other slots are inert.
    static fusion::action_func_pointer actions[] = {
        action_set_value_at_key, // 0
        action_unused,           // 1
        action_unused,           // 2
        action_clear_array,      // 3
        action_unused,           // 4
        action_unused,           // 5
        action_unused,           // 6
        action_unused,           // 7
        action_unused,           // 8
        action_unused,           // 9
        action_unused,           // 10
        action_unused,           // 11
        action_load_ini_file_into_values, // 12
        action_unused,           // 13
        action_save_ini_file_from_values // 14
    };
    static fusion::condition_func_pointer conditions[] = {
        condition_unused, condition_unused, condition_unused, condition_unused,
        condition_unused, condition_unused, condition_unused, condition_unused,
        condition_unused, condition_unused, condition_unused, condition_unused
    };
    static fusion::expression_func_pointer expressions[] = {
        expression_get_value // 0
    };

    run_info->actions = actions;
    run_info->conditions = conditions;
    run_info->expressions = expressions;
    run_info->numOfActions = static_cast<short>(std::size(actions));
    run_info->numOfConditions = static_cast<short>(std::size(conditions));
    run_info->numOfExpressions = static_cast<short>(std::size(expressions));
    return FUSION_GET_EXTENSION_INFO_SUCCESS;
}
#endif
