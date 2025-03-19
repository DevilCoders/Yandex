#pragma once

#include <kernel/factors_info/factors_info.h>

// This header file is auto-generated from factors_gen.in
#include <kernel/facts/factors_info/factors_gen.h>

namespace NUnstructuredFeatures {

    const size_t N_FACTOR_COUNT = FI_FACTOR_COUNT;

    struct TFactFactorStorage : public TGeneralFactorStorage<N_FACTOR_COUNT> {
        TFactFactorStorage() {
            Clear();
        }
    };

} // namespace NUnstructuredFeatures
