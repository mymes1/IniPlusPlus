#include "FusionAPI.hpp"
#include "RunData.hpp"

#include <new>

#ifdef FUSION_GET_RUNTIME_STRUCTURE_SIZE
auto FUSION_API GetRunObjectDataSize(RunHeader* const, SerializedEditData const*) noexcept
    -> std::uint16_t
{
    return static_cast<std::uint16_t>(sizeof(RunData));
}
#endif

#ifdef FUSION_FINISH_RUNTIME_STRUCTURE_CONSTRUCTION
auto FUSION_API CreateRunObject(RunData* const run_data, SerializedEditData const*,
    createObjectInfo* const) noexcept -> std::int16_t
{
    try
    {
        run_data->store = new (std::nothrow) AssArraySC2Store{};
    }
    catch (...)
    {
        run_data->store = nullptr;
    }
    return run_data->store
        ? FUSION_FINISH_RUNTIME_STRUCTURE_CONSTRUCTION_SUCCESS
        : FUSION_FINISH_RUNTIME_STRUCTURE_CONSTRUCTION_FAILURE;
}
#endif

#ifdef FUSION_BEGIN_RUNTIME_STRUCTURE_DESTRUCTION
auto FUSION_API DestroyRunObject(RunData* const run_data, std::int32_t const) noexcept
    -> std::int16_t
{
    delete run_data->store;
    run_data->store = nullptr;
    return FUSION_BEGIN_RUNTIME_STRUCTURE_DESTRUCTION_SUCCESS;
}
#endif

#ifdef FUSION_ON_TICK
auto FUSION_API HandleRunObject(RunData* const) noexcept -> std::int16_t
{
    return FUSION_ON_TICK_STOP_CALLING;
}
#endif

#ifdef FUSION_PAUSE_RUNTIME
auto FUSION_API PauseRunObject(RunData* const) noexcept -> std::int16_t
{
    return FUSION_PAUSE_RUNTIME_SUCCESS;
}
#endif

#ifdef FUSION_UNPAUSE_RUNTIME
auto FUSION_API ContinueRunObject(RunData* const) noexcept -> std::int16_t
{
    return FUSION_UNPAUSE_RUNTIME_SUCCESS;
}
#endif
