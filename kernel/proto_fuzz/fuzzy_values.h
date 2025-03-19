#pragma once

#include <util/generic/string.h>
#include <util/generic/xrange.h>
#include <util/generic/bitops.h>
#include <util/generic/array_size.h>
#include <util/generic/ymath.h>

#include <array>
#include <initializer_list>
#include <type_traits>
#include <limits>

namespace NProtoFuzz {
    template <typename RngType>
    class TFuzzyValues {
        enum class EIntType {
            Zero,
            MinMax,
            PowerOfTwo,
            PowerOfTen,
            Random
        };

        enum class EStrLenType {
            Zero,
            Small,
            Large
        };

        enum class EStrContentType {
            Ascii,
            Filler
        };

        enum class EDoubleType {
            Zero,
            MinMaxNan,
            PowerOfTwo,
            PowerOfTen,
            In01,
            NotIn01
        };

    public:
        using TRng = RngType;

        template <typename EnumType>
        static EnumType GenEnum(
            RngType& rng,
            std::initializer_list<EnumType> values)
        {
            if (Y_UNLIKELY(0 == values.size())) {
                Y_ASSERT(false);
                return EnumType();
            }

            const size_t index = rng.Uniform(values.size());
            return *(values.begin() + index);
        }

        template <typename EnumType>
        static EnumType GenEnum(
            RngType& rng,
            std::initializer_list<std::pair<EnumType, double>> values)
        {
            if (Y_UNLIKELY(0 == values.size())) {
                Y_ASSERT(false);
                return EnumType();
            }

            const double prob = rng.GenRandReal1();
            double acc = 0.0f;
            for (const auto& value : values) {
                const double accNext = acc + value.second;
                if (prob >= accNext) {
                    acc = accNext;
                } else {
                    return value.first;
                }
            }
            return (values.end() - 1)->first;
        }

        template <
            typename ValueType,
            bool AllowZero = true>
        static ValueType GenInt(RngType& rng) {
            static_assert(std::is_integral<ValueType>::value, "");

            static constexpr bool isSigned = std::numeric_limits<ValueType>::is_signed;
            static constexpr size_t numBits = 8 * sizeof(ValueType);
            static constexpr size_t maxBinDigits = (isSigned ? numBits - 1 : numBits);

            static const size_t maxDecDigits = GetNumDecimalDigits(maxBinDigits);

            const EIntType type = GenEnum<EIntType>(rng,
                {{EIntType::Zero, (AllowZero ? 0.05 : 0.0)},
                {EIntType::MinMax, 0.05},
                {EIntType::PowerOfTwo, 0.2},
                {EIntType::PowerOfTen, 0.2},
                {EIntType::Random, 0.5}});

            switch (type) {
                case EIntType::Zero: {
                    return 0;
                }
                case EIntType::MinMax: {
                    const bool isMin = (AllowZero || isSigned) && rng.Uniform(2);
                    return isMin ? Min<ValueType>() : Max<ValueType>();
                }
                case EIntType::PowerOfTen: {
                    const bool isNeg = isSigned && rng.Uniform(2);
                    const size_t exp = rng.Uniform(maxDecDigits);
                    const ValueType res = GetPowerOfTen(exp);
                    return isNeg ? -res : res;
                }
                case EIntType::PowerOfTwo: {
                    const bool isNeg = isSigned && rng.Uniform(2);
                    const size_t exp = rng.Uniform(maxBinDigits);
                    const ValueType res = ui64(1) << exp;
                    return isNeg ? -res : res;
                }
                case EIntType::Random: {
                    const bool isNeg = isSigned && rng.Uniform(2);
                    const size_t part = rng.Uniform(CeilLog2((maxBinDigits + 7) / 8 + 1));
                    const size_t numBits2 = Min(maxBinDigits, size_t(1) << (3 + part)); // 8 * (2 ^ part)
                    const ui64 valueMask = MaskLowerBits(numBits2);
                    const ValueType res = Max(ui64(1), rng.GenRand64() & valueMask);
                    return isNeg ? -res : res;
                }
            }
        }

        template <bool AllowZero = true>
        static double GenDouble(TRng& rng) {
            EDoubleType type = GenEnum<EDoubleType>(rng,
                {{EDoubleType::Zero, (AllowZero ? 0.05 : 0.0)},
                {EDoubleType::MinMaxNan, 0.05},
                {EDoubleType::PowerOfTwo, 0.2},
                {EDoubleType::PowerOfTen, 0.2},
                {EDoubleType::In01, 0.25},
                {EDoubleType::NotIn01, 0.25}});

            switch (type) {
                case EDoubleType::Zero: {
                    return 0.0;
                }
                case EDoubleType::MinMaxNan: {
                    const static double values[] = {NAN, -HUGE_VAL, +HUGE_VAL};
                    return values[rng.Uniform(Y_ARRAY_SIZE(values))];
                }
                case EDoubleType::PowerOfTwo: {
                    const bool isNeg = rng.Uniform(2);
                    const int exp = rng.Uniform(-20, 20);
                    const double res = (exp >= 0 ? double(1UL << exp) : 1.0 / double(1UL << -exp));
                    return (isNeg ? -res : res);
                }
                case EDoubleType::PowerOfTen: {
                    const bool isNeg = rng.Uniform(2);
                    const int exp = rng.Uniform(-10, 10);
                    const double res = (exp >= 0 ? double(GetPowerOfTen(exp)) : 1.0 / double(GetPowerOfTen(-exp)));
                    return (isNeg ? -res : res);
                }
                case EDoubleType::In01: {
                    return 1e-12 + rng.GenRandReal1();
                }
                case EDoubleType::NotIn01: {
                    const bool isNeg = rng.Uniform(2);
                    const double res = 1.0 / Min(1.0, rng.GenRandReal1() + 1e-12);
                    return (isNeg ? -res : res);
                }
            }
        }

        template <bool AllowEmpty = true>
        static TString GenString(
            TRng& rng,
            size_t smallMaxLen,
            size_t largeMaxLen)
        {
            Y_ASSERT(smallMaxLen < largeMaxLen);

            EStrLenType lenType = GenEnum<EStrLenType>(rng,
                {{EStrLenType::Small, 0.5},
                {EStrLenType::Large, 0.5}});

            EStrContentType conType = GenEnum<EStrContentType>(rng,
                {{EStrContentType::Ascii, 0.5},
                {EStrContentType::Filler, 0.5}});

            const size_t len = (lenType == EStrLenType::Small
                ? rng.Uniform(AllowEmpty ? 0 : 1, smallMaxLen + 1)
                : rng.Uniform(smallMaxLen + 1, largeMaxLen + 1));

            TString ret;
            ret.reserve(len);

            for (size_t i : xrange(len)) {
                Y_UNUSED(i);

                char c = 0;
                switch (conType) {
                    case EStrContentType::Ascii: {
                        c = char(rng.Uniform(1, 128));
                        break;
                    }
                    case EStrContentType::Filler: {
                        c = 'X';
                        break;
                    }
                }

                ret.push_back(c);
            }
            return ret;
        }

    private:
        static size_t GetNumDecimalDigits(size_t maxBits) {
            const ui64 maxVal = MaskLowerBits(maxBits);

            size_t k = 0;
            ui64 val = 1;
            while (val <= maxVal) {
                const size_t nextVal = val * ui64(10);
                if (nextVal < val) { // overflow
                    return k;
                }

                k += 1;
                val = nextVal;
            }

            return k - 1;
        }

        static const size_t NumPowersOfTen = 20; // floor(log10(2^64))

        static std::array<ui64, NumPowersOfTen> GetPowersOfTen() {
            std::array<ui64, NumPowersOfTen> res;
            res[0] = 1;
            for (size_t i : xrange(size_t(1), NumPowersOfTen)) {
                res[i] = ui64(10) * res[i - 1];
            }
            return res;
        }

        static size_t GetPowerOfTen(size_t exp) {
            static const std::array<ui64, NumPowersOfTen> powers = GetPowersOfTen();
            return powers[exp];
        }
    };
} // NProtoFuzz
