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
fusion::expression_func expression_unused;
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

    // Keep the original AssArray ABI table extents so Fusion can compare this
    // replacement against existing object metadata. Only the SC2 ACE subset is
    // exposed by Menus.cpp; every other dispatch slot is intentionally inert.
    static fusion::action_func_pointer actions[] = {
        action_set_value_at_key, // 0
        action_unused, // 1
        action_unused, // 2
        action_clear_array, // 3
        action_unused, // 4
        action_unused, // 5
        action_unused, // 6
        action_unused, // 7
        action_unused, // 8
        action_unused, // 9
        action_unused, // 10
        action_unused, // 11
        action_load_ini_file_into_values, // 12
        action_unused, // 13
        action_save_ini_file_from_values, // 14
        action_unused, // 15
        action_unused, // 16
        action_unused, // 17
        action_unused, // 18
        action_unused, // 19
        action_unused, // 20
        action_unused, // 21
        action_unused, // 22
        action_unused, // 23
        action_unused, // 24
        action_unused, // 25
        action_unused, // 26
        action_unused, // 27
        action_unused, // 28
        action_unused, // 29
        action_unused, // 30
        action_unused, // 31
        action_unused, // 32
        action_unused, // 33
        action_unused, // 34
        action_unused, // 35
        action_unused, // 36
        action_unused, // 37
        action_unused, // 38
        action_unused, // 39
        action_unused, // 40
        action_unused, // 41
        action_unused, // 42
        action_unused, // 43
        action_unused, // 44
    };
    static fusion::condition_func_pointer conditions[] = {
        condition_unused, // 0
        condition_unused, // 1
        condition_unused, // 2
        condition_unused, // 3
        condition_unused, // 4
        condition_unused, // 5
        condition_unused, // 6
        condition_unused, // 7
        condition_unused, // 8
        condition_unused, // 9
        condition_unused, // 10
        condition_unused, // 11
        condition_unused, // 12
        condition_unused, // 13
        condition_unused, // 14
        condition_unused, // 15
        condition_unused, // 16
        condition_unused, // 17
        condition_unused, // 18
        condition_unused, // 19
        condition_unused, // 20
        condition_unused, // 21
        condition_unused, // 22
        condition_unused, // 23
        condition_unused, // 24
        condition_unused, // 25
        condition_unused, // 26
    };
    static fusion::expression_func_pointer expressions[] = {
        expression_get_value, // 0
        expression_unused, // 1
        expression_unused, // 2
        expression_unused, // 3
        expression_unused, // 4
        expression_unused, // 5
        expression_unused, // 6
        expression_unused, // 7
        expression_unused, // 8
        expression_unused, // 9
        expression_unused, // 10
        expression_unused, // 11
        expression_unused, // 12
        expression_unused, // 13
        expression_unused, // 14
        expression_unused, // 15
        expression_unused, // 16
        expression_unused, // 17
        expression_unused, // 18
        expression_unused, // 19
        expression_unused, // 20
        expression_unused, // 21
        expression_unused, // 22
        expression_unused, // 23
        expression_unused, // 24
        expression_unused, // 25
        expression_unused, // 26
        expression_unused, // 27
        expression_unused, // 28
        expression_unused, // 29
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
