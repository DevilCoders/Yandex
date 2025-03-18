#pragma once

#include "factor_node.h"

#include <util/generic/set.h>

namespace NSegmentator {
class TFactorTree;

class TAbstractApplicator {
public:
    TAbstractApplicator(TFactorTree& factorTree)
        : FactorTree(factorTree)
    {}

    virtual ~TAbstractApplicator()
    {}

    TFactorTree& GetFactorTree() {
        return FactorTree;
    }

    virtual void Apply(bool useModel = true) = 0;

protected:
    TFactorTree& FactorTree;
};


void CalcExtFactors(TFactors& extFactors, const TFactors& factors, const TSet<ui32>& factorIds,
        size_t numFeats, size_t factorsCount);

}  // NSegmentator
