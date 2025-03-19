#pragma once

#include <util/generic/vector.h>
#include <util/generic/ymath.h>

class TConstFactorView;
struct SRelevanceFormula;

namespace NRelevFml {

struct TMultiple {
    TVector<int> Params;
    float Weight;
    float Value;
};

struct TMultipleMore {
    inline bool operator () (const TMultiple& a, const TMultiple& b) const {
        return fabs(a.Value) > fabs(b.Value);
    }
};

double CalculateMultiples(const SRelevanceFormula& fml, const TConstFactorView& viewFormula, TVector<TMultiple>& multiples, TMultipleMore& multiplesMore);

} // namespace NRelevFml
