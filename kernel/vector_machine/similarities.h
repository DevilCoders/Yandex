#pragma once

#include "common.h"
#include <cmath>
#include <library/cpp/dot_product/dot_product.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NVectorMachine {
    class TCosSimilarity {
    public:
        static float CalcSim(const TEmbed& first, const TEmbed& second) {
            Y_ENSURE(first.size() == second.size(), "Embeds have different sizes.");

            const float dot = DotProduct(first.begin(), second.begin(), first.size());
            const float firstNorm = DotProduct(first.begin(), first.begin(), first.size());
            const float secondNorm = DotProduct(second.begin(), second.begin(), second.size());

            const float norm = sqrt(firstNorm * secondNorm);
            return norm > 0 ? dot / norm : 0;
        }
    };

    class TDotSimilarity{
    public:
        static float CalcSim(const TEmbed& first, const TEmbed& second) {
            Y_ENSURE(first.size() == second.size(), "Embeds have different sizes.");

            return DotProduct(first.begin(), second.begin(), first.size());
        }
    };

    class TDotSimilarityNoSizeCheck {
    public:
        static float CalcSim(const TEmbed& first, const TEmbed& second) {
            if (first.size() == second.size()) {
                return DotProduct(first.begin(), second.begin(), first.size());
            }
            Y_ENSURE(second.empty() || first.empty()); // Embeds have different nonzero sizes
            return 0.f;
        }
    };
} // namespace NVectorMachine
