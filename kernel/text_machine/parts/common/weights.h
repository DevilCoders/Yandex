#pragma once

#include "storage.h"

#include <util/generic/xrange.h>

namespace NTextMachine {
namespace NCore {

using TWeights = TPodBuffer<float>;
using TWeightsHolder = TPoolPodHolder<float>;

class TWeightsHelper {
public:
    inline TWeightsHelper(const TWeights& weights)
        : Weights(weights)
    {
        Weights.Fill(SmoothValue);
    }

    inline void SetWordWeight(size_t wordIndex, float wordWeight) {
        Y_ASSERT(wordWeight >= 0.0f);
        Weights[wordIndex] = wordWeight + SmoothValue;
    }

    void Finish() {
        static const float SmoothValueForSum = SmoothValue * SmoothValue;
        float sumOfWeights = SmoothValueForSum; // To ensure (\sum_i w_i) <= 1.0

        for (size_t i : xrange(Weights.Count())) {
            sumOfWeights += Weights[i];
        }

        for (size_t i : xrange (Weights.Count())) {
            Weights[i] /= sumOfWeights;
            Y_ASSERT(Weights[i] <= 1.0f);
        }
    }

private:
    const float SmoothValue = 1e-6f;
    TWeights Weights;
};

} // NCore
} // NTextMachine
