#include "index_weighter.h"

#include <util/generic/ymath.h>

namespace NEthos {

double TIndexWeighter::CalculateWeight(float index) const {
    if (Factor * index > Factor * Threshold + 15.) {
        return 0.;
    }
    return (1 + std::exp(-Factor * Threshold)) / (1 + std::exp(Factor * index - Factor * Threshold));
}

void TIndexWeighter::Update() {
    size_t precalculatedCount = Max(Threshold * 5, (size_t) 1);

    TVector<double>(precalculatedCount).swap(PrecalculatedWeights);

    for (size_t position = 0; position < precalculatedCount; ++position) {
        PrecalculatedWeights[position] = CalculateWeight(position);
    }
}

}
