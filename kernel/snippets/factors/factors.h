#pragma once

#include <util/system/defaults.h>

#include <kernel/snippets/factors/factors_gen.h>
#include <kernel/snippets/factors/factors_gen_web.h>
#include <kernel/snippets/factors/factors_gen_web_v1.h>
#include <kernel/snippets/factors/factors_gen_web_noclick.h>

namespace NSnippets
{
    using EAlgo2Factors = NSliceSnippetsMain::EFactorId;
    const size_t A2_FACTORS_COUNT = NSliceSnippetsMain::FI_FACTOR_COUNT;
    const size_t FC_ALGO2 = A2_FACTORS_COUNT;
    const size_t FC_ALGO2_BAG = 6;

    extern const TFactorDomain algo2Domain;
    extern const TFactorDomain algo2PlusWebDomain;
    extern const TFactorDomain algo2PlusWebNoClickDomain;
    extern const TFactorDomain algo2PlusWebV1Domain;
}
