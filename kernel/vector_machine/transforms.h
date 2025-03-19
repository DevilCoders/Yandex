#pragma once

#include <cmath>

#include <util/generic/utility.h>

namespace NVectorMachine {
    class TLinearTransform  {
    private:
        float Multiplier = 1.f;
        float Bias = 0.f;

    public:
        TLinearTransform(float multiplier, float bias)
            : Multiplier(multiplier)
            , Bias(bias)
        {}

        float Transform(float value) const {
            return Multiplier * value + Bias;
        }
    };

    class TSigmoidTransform {
    public:

        TSigmoidTransform() = default;

        float Transform(float value) const {
            return 1.f / (1.f + std::exp(-value));
        }
    };

    class TClampTransform {
    private:
        float MinValue = 0.f;
        float MaxValue = 1.f;

    public:
        TClampTransform(float min, float max)
            : MinValue(min)
            , MaxValue(max)
        {}

        float Transform(float value) const {
            return ClampVal<float>(value, MinValue, MaxValue);
        }
    };
} // namespace NVectorMachine
