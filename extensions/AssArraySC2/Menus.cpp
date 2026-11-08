#include "FusionAPI.hpp"
#include "lSDK.hpp"
#include "lSDK/FusionMenu.hpp"
#include "lSDK/FusionUtilities.hpp"

#ifdef FUSION_GET_ACTION_MENU
auto FUSION_API GetActionMenu(mv* const, OI* const, SerializedEditData*) noexcept -> HMENU
{
    using Menu = lSDK::ActionMenu;
    return Menu{{
        Menu::Submenu{TSL("Array Values"), {
            Menu::Item{0, TSL("Set Value at Key")},
            Menu::Item{3, TSL("Clear the Array")}
        }},
        Menu::Submenu{TSL("INI Files"), {
            Menu::Item{12, TSL("Load INI File into Values")},
            Menu::Item{14, TSL("Save INI File from Values")}
        }}
    }}.generate().release();
}
#endif

#ifdef FUSION_GET_CONDITION_MENU
auto FUSION_API GetConditionMenu(mv* const, OI* const, SerializedEditData*) noexcept -> HMENU
{
    // Sonic Classic 2 uses none of the source object's conditions.
    return FUSION_GET_CONDITION_MENU_NONE;
}
#endif

#ifdef FUSION_GET_EXPRESSION_MENU
auto FUSION_API GetExpressionMenu(mv* const, OI* const, SerializedEditData*) noexcept -> HMENU
{
    using Menu = lSDK::ExpressionMenu;
    return Menu{{Menu::Item{0, TSL("GetValue")}}}.generate().release();
}
#endif

#ifdef FUSION_GET_ACTION_ID_FROM_ACTION_MENU_ID
auto FUSION_API GetActionCodeFromMenu(mv* const, std::int16_t const menu_id) noexcept -> std::int16_t
{
    return lSDK::ActionMenu::ExtensionData::from_menu_id(menu_id);
}
#endif

#ifdef FUSION_GET_CONDITION_ID_FROM_CONDITION_MENU_ID
auto FUSION_API GetConditionCodeFromMenu(mv* const, std::int16_t const menu_id) noexcept -> std::int16_t
{
    return lSDK::ConditionMenu::ExtensionData::from_menu_id(menu_id);
}
#endif

#ifdef FUSION_GET_EXPRESSION_ID_FROM_EXPRESSION_MENU_ID
auto FUSION_API GetExpressionCodeFromMenu(mv* const, std::int16_t const menu_id) noexcept -> std::int16_t
{
    return lSDK::ExpressionMenu::ExtensionData::from_menu_id(menu_id);
}
#endif

#ifdef FUSION_GET_ACTION_INFO_ARRAY
auto FUSION_API GetActionInfos(mv* const, std::int16_t const action_id) noexcept -> fusion::ace_info_array
{
    switch (action_id)
    {
    case 0:
        // Exact original signature: PARAM_EXPSTRING key, PARAM_EXPRESSION numeric value.
        return lSDK::constant_ace_info<0, 0, PARAM_EXPSTRING, PARAM_EXPRESSION>();
    case 3:
        return lSDK::constant_ace_info<3, 0>();
    case 12:
        return lSDK::constant_ace_info<12, 0, PARAM_EXPSTRING>();
    case 14:
        return lSDK::constant_ace_info<14, 0, PARAM_EXPSTRING>();
    default:
        return nullptr;
    }
}
#endif

#ifdef FUSION_GET_CONDITION_INFO_ARRAY
auto FUSION_API GetConditionInfos(mv* const, std::int16_t const) noexcept -> fusion::ace_info_array
{
    return nullptr;
}
#endif

#ifdef FUSION_GET_EXPRESSION_INFO_ARRAY
auto FUSION_API GetExpressionInfos(mv* const, std::int16_t const expression_id) noexcept -> fusion::ace_info_array
{
    if (expression_id == 0)
    {
        // Exact original signature: one EXPPARAM_STRING key.
        return lSDK::constant_ace_info<0, 0, EXPPARAM_STRING>();
    }
    return nullptr;
}
#endif

#ifdef FUSION_GET_ACTION_PARAM_NAME
void FUSION_API GetActionTitle(mv* const, std::int16_t const action_id, std::int16_t const param_id,
    fusion::string_buffer const buffer, std::int16_t const buffer_size) noexcept
{
    switch (action_id)
    {
    case 0:
        if (param_id == 0)
        {
            (void)lSDK::fill_buffer(buffer, buffer_size, TSL("Enter the Key"));
        }
        else if (param_id == 1)
        {
            (void)lSDK::fill_buffer(buffer, buffer_size, TSL("Enter the Value"));
        }
        return;
    case 12:
        if (param_id == 0)
        {
            (void)lSDK::fill_buffer(buffer, buffer_size, TSL("Enter the File to Load"));
        }
        return;
    case 14:
        if (param_id == 0)
        {
            (void)lSDK::fill_buffer(buffer, buffer_size, TSL("Enter the File to Save"));
        }
        return;
    default:
        return;
    }
}
#endif

#ifdef FUSION_GET_CONDITION_PARAM_NAME
void FUSION_API GetConditionTitle(mv* const, std::int16_t const, std::int16_t const,
    fusion::string_buffer const buffer, std::int16_t const buffer_size) noexcept
{
    (void)lSDK::fill_buffer(buffer, buffer_size, TSL(""));
}
#endif

#ifdef FUSION_GET_EXPRESSION_PARAM_NAME
void FUSION_API GetExpressionParam(mv* const, std::int16_t const expression_id, std::int16_t const param_id,
    fusion::string_buffer const buffer, std::int16_t const buffer_size) noexcept
{
    if (expression_id == 0 && param_id == 0)
    {
        (void)lSDK::fill_buffer(buffer, buffer_size, TSL("Enter the Key"));
    }
}
#endif

#ifdef FUSION_GET_ACTION_DISPLAY_STRING
void FUSION_API GetActionString(mv* const, std::int16_t const action_id,
    fusion::string_buffer const buffer, std::int16_t const buffer_size) noexcept
{
    switch (action_id)
    {
    case 0:
        (void)lSDK::fill_buffer(buffer, buffer_size, TSL("%o: Set Value at Key %0 to %1"));
        return;
    case 3:
        (void)lSDK::fill_buffer(buffer, buffer_size, TSL("%o: Clear the Array"));
        return;
    case 12:
        (void)lSDK::fill_buffer(buffer, buffer_size, TSL("%o: Load INI File %0 into Values"));
        return;
    case 14:
        (void)lSDK::fill_buffer(buffer, buffer_size, TSL("%o: Save INI File %0 from Values"));
        return;
    default:
        (void)lSDK::fill_buffer(buffer, buffer_size, TSL(""));
        return;
    }
}
#endif

#ifdef FUSION_GET_CONDITION_DISPLAY_STRING
void FUSION_API GetConditionString(mv* const, std::int16_t const,
    fusion::string_buffer const buffer, std::int16_t const buffer_size) noexcept
{
    (void)lSDK::fill_buffer(buffer, buffer_size, TSL(""));
}
#endif

#ifdef FUSION_GET_EXPRESSION_EDITOR_STRING
void FUSION_API GetExpressionString(mv* const, std::int16_t const expression_id,
    fusion::string_buffer const buffer, std::int16_t const buffer_size) noexcept
{
    if (expression_id == 0)
    {
        (void)lSDK::fill_buffer(buffer, buffer_size, TSL("GetValue("));
    }
}
#endif
