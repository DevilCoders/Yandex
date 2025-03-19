#pragma once

#include "types.h"

namespace NTextMachine {
namespace NCore {
    inline float GetNormalizedValue(float x) {
        return isnan(x) ? 0.0f : Max<float>(0.0f, Min<float>(1.0f, x));
    }

    inline void VerifyAndFixFeatures(const TFloatsBuffer& values) {
        for (float& value : values) {
            Y_VERIFY_DEBUG(value >= 0.0f - FloatEpsilon, "Feature is < 0.0");
            Y_VERIFY_DEBUG(value <= 1.0f + FloatEpsilon, "Feature is > 1.0");
            Y_VERIFY_DEBUG(!isnan(value), "Feature is NaN");

            value = GetNormalizedValue(value);
        }
    }
} // NCore
} // NTextMachine
