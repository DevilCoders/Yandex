#pragma once

#include <util/memory/blob.h>
#include <util/generic/vector.h>

namespace NVowpalWabbit {
    class TModel {
    private:
        TBlob Data;
        const float* const Weights;
        const ui8 Bits;
        const ui32 HashMask;

    public:
        explicit TModel(const TString& flatWeightFileName);
        explicit TModel(const TBlob& weights);
        [[nodiscard]]
        const float* GetWeights() const;
        [[nodiscard]]
        size_t GetWeightsSize() const;
        [[nodiscard]]
        ui8 GetBits() const;

        static void ConvertReadableModel(const TString& readableModelFileName, const TString& flatWeightFileName);

        [[nodiscard]]
        const TBlob& GetBlob() const;

        float operator[] (ui32 hash) const {
            return Weights[hash & HashMask];
        }

        float operator[] (const TVector<ui32>& hashes) const {
            float prediction = 0;
            for (ui32 hash : hashes) {
                prediction += Weights[hash & HashMask];
            }

            return prediction;
        }

    private:
        void CheckBlob();
    };

    TModel ReadTextModel(IInputStream& input);
    TModel ReadTextModel(const TString& fileName);

    /**
     * Packed model uses 1 byte per weight, so its size 4 times smaller comparing to float model.
     * Float model can be converted to packed model using Nirvana operation:
     * https://nirvana.yandex-team.ru/operation/da765175-aaa1-44c2-9914-43f37d6fba53/overview
     */
    class TPackedModel {
    private:
        TBlob Data;
        float Multiplier;
        const i8 * const Weights = nullptr;
        const size_t Size;
        const ui8 Bits;
        const ui32 HashMask;

    public:
        explicit TPackedModel(const TString& modelFileName);
        explicit TPackedModel(const TBlob& model);

        [[nodiscard]]
        const i8* GetWeights() const {
            return Weights;
        }

        [[nodiscard]]
        size_t GetWeightsSize() const {
            return Size;
        }

        [[nodiscard]]
        ui8 GetBits() const {
            return Bits;
        }

        float operator[] (ui32 hash) const {
            return Weights[hash & HashMask] * Multiplier;
        }

        float operator[] (const TVector<ui32>& hashes) const {
            i64 prediction = 0;
            for (ui32 hash : hashes) {
                prediction += Weights[hash & HashMask];
            }

            return prediction * Multiplier;
        }

        [[nodiscard]]
        float GetMultiplier() const {
            return Multiplier;
        }
    };
}

using TVowpalWabbitModel = NVowpalWabbit::TModel;
