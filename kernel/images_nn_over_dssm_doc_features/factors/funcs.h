#pragma once

#include <util/datetime/base.h>

namespace NImagesNnOverDssmDocFeatures {

    float CalcTimeFactorDays(const TInstant& requestTime, const TInstant& docTime);
    float CalcTimeFactorMinutes(const TInstant& requestTime, const TInstant& docTime);
    float ImageAreaSqrtSigmoid(float area);
    float ImageAreaSigmoid(float area);

}  // namespace NImagesNnOverDssmDocFeatures
