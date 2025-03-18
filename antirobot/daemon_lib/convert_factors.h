#pragma once

#include "factors.h"

namespace NAntiRobot {
    void InitFactorsConvert();
    void ConvertFactors(ui32 fromVersion, const float* srcFactors, TFactorsWithAggregation& dest);
}
