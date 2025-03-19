#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/vector.h>

#include <array>

namespace NDssmApplier {
    using TDecompression = std::array<float, 256>;
    namespace {
        template<class TFunction, size_t... Indices>
        constexpr auto MakeArrayHelper(TFunction f, std::index_sequence<Indices...>) noexcept -> std::array<decltype(f(size_t{})), sizeof...(Indices)> {
            return {{ f(Indices)... }};
        }
    }

    namespace NUtils {
        // Сжатие [-x;y] -> [0; 255] при этом 0 -> ~128
        TVector<ui8> Compress(TConstArrayRef<float> numbers, float minimum, float maximum) noexcept;
        void Decompress(TConstArrayRef<ui8> compressed, const TDecompression& decompression, TVector<float>* result) noexcept;
        TVector<float> Decompress(TConstArrayRef<ui8> compressed, const TDecompression& decompression) noexcept;

        // Сжатие без смещения относительно нуля (т.е. просто умножение на число)
        TVector<i8> Scale(TConstArrayRef<float> numbers, float minimum, float maximum) noexcept;
        TVector<float> UnScale(TConstArrayRef<i8> numbers, float minimum, float maximum) noexcept;

        template<size_t N, class TFunction>
        constexpr auto MakeArray(TFunction f) noexcept {
            return MakeArrayHelper(f, std::make_index_sequence<N>{});
        }

        constexpr TDecompression GenerateSingleDecompression(const float min, const float max) noexcept {
            auto makeBucket = [=](const size_t index) {
                const float step = (max - min) / std::tuple_size<TDecompression>::value;
                return min + step * index + 0.5f * step;
            };
            return MakeArray<std::tuple_size<TDecompression>::value>(makeBucket);
        }

        struct TFloat2UI8Compressor {
            static TVector<ui8> Compress(TConstArrayRef<float> vect, float* multipler = nullptr) noexcept;
            static TVector<float> Decompress(TConstArrayRef<ui8> vect) noexcept;
        };
    }
}

namespace NNeuralNetApplier {
    Y_PURE_FUNCTION
    float GetL2Norm(TConstArrayRef<float> data) noexcept;
    Y_PURE_FUNCTION
    float GetMaxNorm(TConstArrayRef<float> data) noexcept;
    Y_PURE_FUNCTION
    bool IsNormalized(TConstArrayRef<float> embedding) noexcept;
    bool TryNormalize(TArrayRef<float> embedding) noexcept;
    bool TryNormalize(TArrayRef<float> embedding, float norm) noexcept;
    float NormalizeVector(TArrayRef<float> vec) noexcept;
    bool MaxNormalizeVector(TArrayRef<float> vec) noexcept;
    TString GetNameWithVersion(const TString& name, const TString& version);
}
