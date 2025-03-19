#include "optimization_options.h"
#include <util/system/yassert.h>
#include <library/cpp/logger/global/global.h>

namespace NRTYArchive {

    TOptimizationOptions& TOptimizationOptions::SetPopulationRate(float rate) {
        VERIFY_WITH_LOG(rate > 0.0f && rate < 1.0f, "invalid rate %f", rate);
        PopulationRate = rate;
        return *this;
    }

    TOptimizationOptions& TOptimizationOptions::SetPartSizeDeviation(float deviation) {
        VERIFY_WITH_LOG(deviation > 0.0f && deviation < 1.0f, "invalid rate %f", deviation);
        PartSizeDeviation = deviation;
        return *this;
    }

    TOptimizationOptions& TOptimizationOptions::SetMaxUndersizedPartsCount(ui32 maxUndersizedPartsCount) {
        MaxUndersizedPartsCount = maxUndersizedPartsCount;
        return *this;
    }

    float TOptimizationOptions::GetPopulationRate() const {
        return PopulationRate;
    }

    float TOptimizationOptions::GetPartSizeDeviation() const {
        return PartSizeDeviation;
    }

    ui32 TOptimizationOptions::GetMaxUndersizedPartsCount() const {
        return MaxUndersizedPartsCount;
    }
}
