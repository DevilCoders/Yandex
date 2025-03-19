#include "common.h"

namespace NFacts {
    float CalcMean(const TVector<float>& snippetFactors) {
        if (snippetFactors.size() == 0) {
            return 0.f;
        }
        float sum = 0.f;
        for (size_t i = 0; i < snippetFactors.size(); ++i) {
            sum += snippetFactors[i];
        }
        return sum / snippetFactors.size();
    }
}
