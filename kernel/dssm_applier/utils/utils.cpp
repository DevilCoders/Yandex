#include "utils.h"

#include <library/cpp/dot_product/dot_product.h>

#include <util/generic/algorithm.h>
#include <util/generic/utility.h>
#include <util/generic/ymath.h>
#include <util/str_stl.h>
#include <util/system/yassert.h>

#include <cmath>

namespace NDssmApplier {
    namespace NUtils {
        TVector<ui8> Compress(const TConstArrayRef<float> numbers, const float minimum, const float maximum) noexcept {
            Y_ASSERT(minimum < maximum);
            auto multiplier = std::nextafter(256.0f / (maximum - minimum), -std::numeric_limits<float>::max());

            TVector<ui8> res(Reserve(numbers.size()));
            for (float number : numbers) {
                number = std::clamp(number, minimum, maximum);
                res.emplace_back((number - minimum) * multiplier);
            }

            return res;
        }

        void Decompress(const TConstArrayRef<ui8> compressed, const TDecompression& decompression, TVector<float>* const result) noexcept {
            result->clear();
            result->reserve(compressed.size());
            for (ui8 idx : compressed) {
                result->push_back(decompression[idx]);
            }
        }

        TVector<float> Decompress(const TConstArrayRef<ui8> compressed, const TDecompression& decompression) noexcept {
            TVector<float> result;
            Decompress(compressed, decompression, &result);
            return result;
        }

        double GetScaleMultiplier(const float minimum, const float maximum) noexcept {
            const auto deviation = std::max(std::abs(maximum), std::abs(minimum));
            const auto multiplier = std::nextafter(127.0f / deviation, -std::numeric_limits<float>::max());
            return multiplier;
        }

        TVector<i8> Scale(const TConstArrayRef<float> numbers, const float minimum, const float maximum) noexcept {
            Y_ASSERT(minimum < maximum);
            const auto multiplier = GetScaleMultiplier(minimum, maximum);

            TVector<i8> res(Reserve(numbers.size()));
            for (float number : numbers) {
                number = Max(number, minimum);
                number = Min(number, maximum);
                res.emplace_back(static_cast<i8>(std::round(number * multiplier)));
            }

            return res;
        }

        TVector<float> UnScale(const TConstArrayRef<i8> numbers, const float minimum, const float maximum) noexcept {
            Y_ASSERT(minimum < maximum);
            const auto multiplier = 1.0 / GetScaleMultiplier(minimum, maximum);
            TVector<float> res(Reserve(numbers.size()));
            for (const float number : numbers) {
                res.emplace_back(static_cast<float>(number * multiplier));
            }
            return res;
        }

        TVector<ui8> TFloat2UI8Compressor::Compress(const TConstArrayRef<float> vect, float* const multipler) noexcept {
            if (vect.empty()) {
                return {};
            }

            auto maxAbs = fabs(*std::max_element(vect.begin(), vect.end(), [](float lhs, float rhs) {
                return fabs(lhs) < fabs(rhs);
            }));
            if (maxAbs == 0)
                maxAbs = std::nextafter(maxAbs, std::numeric_limits<float>::max());
            if (multipler) {
                *multipler = maxAbs;
            }

            return NUtils::Compress(vect, -1.0f * maxAbs, 1.0f * maxAbs);
        }

        TVector<float> TFloat2UI8Compressor::Decompress(const TConstArrayRef<ui8> vect) noexcept {
            if (vect.empty()) {
                return {};
            }

            static auto decompression = GenerateSingleDecompression(-1.0f, 1.0f);
            return NUtils::Decompress(vect, decompression);
        }
    }
}

namespace NNeuralNetApplier {
    float GetL2Norm(TConstArrayRef<float> data) noexcept {
        return std::sqrtf(L2NormSquared(data.data(), data.size()));
    }

    float GetMaxNorm(TConstArrayRef<float> data) noexcept {
        if (data.empty()) {
            return 0.0f;
        } else {
            return std::abs(*MaxElementBy(data, [](const float elem) {return std::abs(elem);}));
        }
    }

    bool IsNormalized(TConstArrayRef<float> embedding) noexcept {
        const float norm = GetL2Norm(embedding);
        return FuzzyEquals(1.0f, norm);
    }

    bool TryNormalize(TArrayRef<float> embedding) noexcept {
        const float norm = GetL2Norm(embedding);
        return TryNormalize(embedding, norm);
    }

    bool TryNormalize(TArrayRef<float> embedding, const float norm) noexcept {
        if (norm < std::numeric_limits<float>::epsilon()) {
            return false;
        }
        const float inv = 1.0f / norm;
        for (float& x : embedding) {
            x *= inv;
        }
        return true;
    }

    float NormalizeVector(TArrayRef<float> vec) noexcept {
        const float len = GetL2Norm(vec);
        if (!TryNormalize(vec, len)) {
            return 0.0f;
        }
        return len;
    }

    bool MaxNormalizeVector(TArrayRef<float> vec) noexcept {
       const float norm = GetMaxNorm(vec);
       return TryNormalize(vec, norm);
    }

    TString GetNameWithVersion(const TString& name, const TString& version) {
        return TString::Join(name, "_v_", version);
    }
}
