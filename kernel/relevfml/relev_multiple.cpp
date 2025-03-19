#include "relev_multiple.h"

#include <util/generic/vector.h>
#include <util/generic/algorithm.h>

#include "relev_fml.h"
#include <kernel/factor_storage/factor_storage.h>

namespace NRelevFml {

double CalculateMultiples(const SRelevanceFormula& fml, const TConstFactorView& viewFormula, TVector<TMultiple>& multiples, TMultipleMore& multiplesMore) {
    TVector<TVector<int>> params;
    TVector<float> weights;
    fml.GetFormula(&params, &weights);

    double totalSum = .0;

    for (size_t i = 0; i < params.size(); ++i) {
        float curMultiple = weights[i];
        for (size_t j = 0; j < params[i].size(); ++j) {
            curMultiple *= viewFormula[(size_t)params[i][j]];
        }
        if (fabs(curMultiple) > 1e-6) {
            TMultiple multiple;
            multiple.Params = params[i];
            multiple.Weight = weights[i];
            multiple.Value = curMultiple;
            multiples.push_back(multiple);
        }
        totalSum += curMultiple;
    }

    Sort(multiples.begin(), multiples.end(), multiplesMore);
    return totalSum;
}

} // namespace NRelevFml
