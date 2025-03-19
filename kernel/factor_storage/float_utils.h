#pragma once

#include <util/generic/utility.h>
#include <util/generic/ymath.h>
#include <util/system/yassert.h>

inline float MapFactor(float f, float fRef1) {
    f *= fRef1;
    float result = f / (1.f + f);
    Y_ASSERT(result >= 0.f);
    Y_ASSERT(result <= 1.f);
    return result;
}

template <unsigned char NumBits>
class TUi2Float {
public:
    static_assert(NumBits > 0, "NumBits must be greater than 0");
    static constexpr ui64 MaxValue = ui64(1) << NumBits;
    static constexpr float Multiplier = float(1.0 / (MaxValue - 1));
    static_assert(NumBits < sizeof(MaxValue) * 8, "NumBits must be less than bits count in MaxValue");

    constexpr TUi2Float() noexcept = default;

    float operator()(ui64 value) const noexcept {
        Y_ASSERT(value < MaxValue);
        return value * Multiplier;
    }
};

constexpr TUi2Float<16> Ui162Float{};
constexpr TUi2Float<8> Ui82Float{};
constexpr TUi2Float<6> Ui62Float{};
constexpr TUi2Float<5> Ui52Float{};
constexpr TUi2Float<4> Ui42Float{};
constexpr TUi2Float<3> Ui32Float{};
constexpr TUi2Float<2> Ui22Float{};
constexpr TUi2Float<1> Bool2Float{};

class TSoftFloatClipper {
    float Lower_ = 0.0f;
    float Upper_ = 1.0f;
    float AbsTolerance_ = 1e-6f;
    float Value_ = NAN;

public:
    explicit TSoftFloatClipper(float value)
        : Value_(value)
    {}

    TSoftFloatClipper& Bounds(float lower, float upper) {
        Lower_ = lower;
        Upper_ = upper;
        return *this;
    }

    TSoftFloatClipper& AbsTolerance(float tolerance) {
        AbsTolerance_ = tolerance;
        return *this;
    }

    float Get() const {
        if (IsFinite(Value_)
            && Value_ - Lower_ >= -AbsTolerance_
            && Value_ - Upper_ <= AbsTolerance_)
        {
            return Max(Lower_, Min(Upper_, Value_));
        }
        return Value_;
    }

    operator float () const {
        return Get();
    }
};

inline TSoftFloatClipper SoftClipFloat(float value) {
    return TSoftFloatClipper(value);
}

inline float SoftClipFloatTo01(float value, float tolerance = 1e-6f) {
    return SoftClipFloat(value).AbsTolerance(tolerance);
}

inline float MapLinear(float x, float max, float delta) {
    return Min(delta, Max(0.f, x - max + delta)) / delta;
}
