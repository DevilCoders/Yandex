#pragma once

#include <kernel/web_factors_info/factor_names.h>

namespace NYellownessFactors {

    static constexpr std::array<ui32, 5> FactorIndices{{
        NSliceWebProduction::FI_YELLOWNESS_MAX,
        NSliceWebProduction::FI_YELLOWNESS_MEAN,
        NSliceWebProduction::FI_YELLOWNESS_MEDIAN,
        NSliceWebProduction::FI_YELLOWNESS_MIN,
        NSliceWebProduction::FI_YELLOWNESS_DISPERSION,
    }};

    using TFactorNames = std::array<const char*, FactorIndices.size()>;

    const TFactorNames& GetFactorsNames();

} // NYellownessFactors
