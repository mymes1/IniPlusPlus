#include "FusionAPI.hpp"
#include "lSDK.hpp"

#ifndef FUSION_RUNTIME_ONLY
#ifdef FUSION_GET_EXTENSION_METADATA
void FUSION_API GetObjInfos(mv* const, SerializedEditData*,
    fusion::string_buffer const name_buffer,
    fusion::string_buffer const author_buffer,
    fusion::string_buffer const copyright_buffer,
    fusion::string_buffer const description_buffer,
    fusion::string_buffer const website_buffer) noexcept
{
    (void)lSDK::fill_buffer(name_buffer, 255, TSL("AssArray SC2 Compatible"));
    (void)lSDK::fill_buffer(author_buffer, 255, TSL("Ini++ Project contributors"));
    (void)lSDK::fill_buffer(copyright_buffer, 255, TSL("2026 Ini++ Project contributors"));
    (void)lSDK::fill_buffer(description_buffer, 1024,
        TSL("Independent AssArray-compatible object exposing the five ACEs used by Sonic Classic 2."));
    (void)lSDK::fill_buffer(website_buffer, 255, TSL("https://github.com/mymes1/IniPlusPlus"));
}
#endif
#endif
