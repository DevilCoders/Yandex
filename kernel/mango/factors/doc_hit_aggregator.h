#pragma once

#include "factors.h"
#include "prel.h"
#include "tr_base.h"

namespace NMango {

    class TDocHitAggregator {
    public:
        void SetQueryInfo(const TQueryWordInfos* queryWordInfos);
        void OnResourceStart(const float* weightyWordWeights);

        void OnHit(const THitInfo& hit);

        void GetFactors(TAccessor<GROUP_DOC_RELEVANCE> docRelevanceFactors,
            TConstAccessor<GROUP_QUOTE_STATIC> quoteStaticFactors) const;

        float GetRelevance(size_t linkCount) const;

    private:
        bool QueryWeightIsNonZero() const;

        TPositionalRelevance PosRelevance;
        const TQueryWordInfos* QueryWordInfos;
        size_t QueryLength;
        float QueryNormWeight;
        TVector<ui32> FlatTF;
    };

}
