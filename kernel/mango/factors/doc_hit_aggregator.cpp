#include "defs.h"
#include "doc_hit_aggregator.h"

#include <kernel/mango/common/types.h>

namespace NMango {

    namespace {
        float inline DocFreqToIdf(float docFreq) {
            return -log(docFreq);
        }

        float FlatRelevance(const TQueryWordInfos& infos, const TVector<ui32>& tfs) {
            static const float MAGIC_K1 = 2.2f;
            float res = 0.0f;
            size_t queryLength = 0;
            for (size_t i = 0; i < infos.size(); ++i) {
                const TQueryWordInfo& qwi = infos[i];
                if (!qwi.IsStopWord) {
                    ++queryLength;
                    ui32 tf = tfs[i];
                    res += tf * DocFreqToIdf(infos[i].DocFreq) / (tf + MAGIC_K1);
                }
            }
            if (!queryLength) {
                return 0.0f;
            }
            return 0.5f + res / (2 * queryLength);
        }
    }

    void TDocHitAggregator::SetQueryInfo(const TQueryWordInfos* queryWordInfos) {
        QueryWordInfos = queryWordInfos;
        QueryLength = 0;
        for (TQueryWordInfos::const_iterator qwi = queryWordInfos->begin(); qwi != queryWordInfos->end(); ++qwi) {
            if (!qwi->IsStopWord) {
                ++QueryLength;
            }
        }
        FlatTF.assign(queryWordInfos->size(), 0);
    }

    void TDocHitAggregator::OnResourceStart(const float* weightyWordWeights) {
        PosRelevance.OnResourceStart();
        FlatTF.assign(QueryWordInfos->size(), 0);
        QueryNormWeight = 0.0f;
        for (size_t i = 0; i < Min(QueryLength, MAX_RELEVANCE_WEIGHTY_WORDS); ++i) {
            QueryNormWeight += weightyWordWeights[i];
        }
    }

    void TDocHitAggregator::OnHit(const THitInfo& hit) {
        const TQueryWordInfo& qwi = (*QueryWordInfos)[hit.QueryWordIndex];
        if (!qwi.IsStopWord) {
            PosRelevance.AddHit(DocFreqToIdf(qwi.DocFreq), hit.WordPosition);
        }
        ++FlatTF[hit.QueryWordIndex];
    }

    void TDocHitAggregator::GetFactors(TAccessor<GROUP_DOC_RELEVANCE> docRelevanceFactors,
        TConstAccessor<GROUP_QUOTE_STATIC> quoteStaticFactors) const
    {
        if (QueryWeightIsNonZero()) {
            size_t quoteCount = quoteStaticFactors[LINKS_COUNT];

            docRelevanceFactors[POS_RELEVANCE]                  = PosRelevance.GetRelevance(quoteCount);
            docRelevanceFactors[POS_RELEVANCE_NORM_BY_DOC]      =
                Min(docRelevanceFactors[POS_RELEVANCE] / QueryNormWeight, 1.0f);
            docRelevanceFactors[POS_RELEVANCE_NORM_BY_QUERY]    =
                docRelevanceFactors[POS_RELEVANCE] / QueryLength;
            docRelevanceFactors[FLAT_RELEVANCE] = FlatRelevance(*QueryWordInfos, FlatTF);
        }
    }

    float TDocHitAggregator::GetRelevance(size_t linkCount) const {
        return QueryWeightIsNonZero() ? Min(PosRelevance.GetRelevance(linkCount) / QueryNormWeight, 1.0f) : 0.0f;
    }

    bool TDocHitAggregator::QueryWeightIsNonZero() const {
        return QueryNormWeight > 1e-5;
    }


}
