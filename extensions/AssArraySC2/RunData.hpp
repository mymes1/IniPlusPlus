#pragma once

#include "AssArraySC2Store.hpp"
#include "FusionAPI.hpp"

struct RunData final
{
    headerObject rHo;
    rVal rv;
    AssArraySC2Store* store;

    // Fusion zero-initializes this memory; non-trivial state lives behind the owned pointer.
    RunData() = delete;
    RunData(RunData const&) = delete;
    RunData(RunData&&) = delete;
    RunData& operator=(RunData const&) = delete;
    RunData& operator=(RunData&&) = delete;
    ~RunData() = delete;
};

static_assert(fusion::is_valid_rundata_structure<FUSION_OEFLAGS>());
