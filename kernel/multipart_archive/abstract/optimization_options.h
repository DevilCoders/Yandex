#pragma once

#include <util/system/types.h>

namespace NRTYArchive {

    class TOptimizationOptions {
    public:
        TOptimizationOptions& SetPopulationRate(float rate);
        TOptimizationOptions& SetPartSizeDeviation(float deviation);
        TOptimizationOptions& SetMaxUndersizedPartsCount(ui32 maxUndersizedPartsCount);

        float GetPopulationRate() const;
        float GetPartSizeDeviation() const;
        ui32 GetMaxUndersizedPartsCount() const;

    private:
        float PopulationRate = 0.6f;
        float PartSizeDeviation = 0.2f;
        ui32 MaxUndersizedPartsCount = 0;
    };

}
