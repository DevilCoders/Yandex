#include "prel.h"

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>

namespace NMango {

    const float TPositionalRelevance::MAGIC_K1 = 1.8f;

    void TPositionalRelevance::OnResourceStart() {
        HitWeightSum = 0.0f;
    }

    void TPositionalRelevance::AddHit(float idf, size_t hitPos) {
        float weight    = idf / sqrt(hitPos + 1.0f);

        HitWeightSum += idf * (weight / (weight + MAGIC_K1));
    }

    float TPositionalRelevance::GetRelevance(size_t quoteCount) const {
        return HitWeightSum / quoteCount;
    }

}
