#include "decompression.h"

namespace NDssmApplier {
    TVector<ui8> Compress(const TArrayRef<const float>& numbers, EDssmModelType modelType) noexcept {
        const auto& bounds = GetBounds(modelType);
        return NUtils::Compress(numbers, bounds.first, bounds.second);
    }

    TVector<float> Decompress(const TString& embedding, const EDssmModelType modelType) noexcept {
        const ui8* dataPtr = reinterpret_cast<const ui8*>(embedding.data());
        const TArrayRef<const ui8> compressedEmb(dataPtr, embedding.size());
        TVector<float> res = NUtils::Decompress(compressedEmb, GetDecompression(modelType));

        return res;
    }

    TVector<i8> Scale(const TArrayRef<const float>& numbers, EDssmModelType modelType) noexcept {
        const auto& bounds = GetBounds(modelType);
        return NUtils::Scale(numbers, bounds.first, bounds.second);
    }

    TVector<float> UnScale(const TArrayRef<const i8>& numbers, EDssmModelType modelType) noexcept {
        const auto& bounds = GetBounds(modelType);
        return NUtils::UnScale(numbers, bounds.first, bounds.second);
    }

}  // namespace NDssmApplier
