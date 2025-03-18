#include "applicator.h"

namespace NSegmentator {

// TODO: temporary solution until 100-factors formulas
void CalcExtFactors(TFactors& extFactors, const TFactors& factors, const TSet<ui32>& factorIds,
        size_t numFeats, size_t factorsCount)
{
    Y_ASSERT(factors.size() == factorsCount);
    Y_ASSERT(factorIds.size() <= factors.size()); // formula may use not all factors
    Y_ASSERT(extFactors.empty());

    extFactors.resize(numFeats, 0);
    TSet<ui32>::const_iterator factorId = factorIds.begin();
    for (size_t i = 0; i < factorsCount && factorId != factorIds.end(); ++i, ++factorId) {
        extFactors[*factorId] = factors[i];
    }
}

}  // NSegmentator
