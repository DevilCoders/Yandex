#pragma once

#include "bag_of_words.h"
#include "types.h"
#include "weights.h"
#include "storage.h"

namespace NTextMachine {
namespace NCore {
    class TBagOfWords;

    class TQueriesHelper {
    public:
        TQueriesHelper(TMemoryPool& pool,
            const TMultiQuery& multiQuery,
            const TExpansionRemap& expansionRemap)
            : MultiQuery(&multiQuery)
        {
            FillMultiQueryInfo(pool, expansionRemap);
        }

        const TWeights& GetMainWeights(size_t queryIdx) const {
            return (*QueryInfo[queryIdx].MainWeightsByType)[TRevFreq::Default];
        }
        const TWeights& GetExactWeights(size_t queryIdx) const {
            return (*QueryInfo[queryIdx].ExactWeightsByType)[TRevFreq::Default];
        }

        const TWeights& GetMainWeights(EExpansionType expansion, size_t queryIdx) const {
            return (*ExpInfo[expansion].Infos[queryIdx]->MainWeightsByType)[TRevFreq::Default];
        }
        const TWeights& GetExactWeights(EExpansionType expansion, size_t queryIdx) const {
            return (*ExpInfo[expansion].Infos[queryIdx]->ExactWeightsByType)[TRevFreq::Default];
        }

        size_t GetNumBlocks() const {
            return NumBlocks;
        }

        size_t GetLocalIndex(size_t queryIdx) const {
            return QueryInfo[queryIdx].LocalIndex;
        }
        size_t GetIndex(EExpansionType expansion, size_t queryLocalIdx) const {
            return ExpInfo[expansion].Infos[queryLocalIdx]->Index;
        }
        size_t GetOriginalQueryIndex() const {
            Y_ASSERT(OriginalQueryIndex.Defined());
            return OriginalQueryIndex.GetOrElse(0);
        }

        size_t GetNumQueries(EExpansionType expansion) const {
            return ExpInfo[expansion].NumQueries;
        }
        const TPoolPodHolder<const TQuery*>& GetQueries(EExpansionType expansion) const {
            return ExpInfo[expansion].Queries;
        }
        size_t GetNumWords(EExpansionType expansion) const {
            return ExpInfo[expansion].NumWords;
        }

        const TMultiQuery& GetMultiQuery() const {
            return *MultiQuery;
        }
        const TQuery& GetQuery(size_t queryIdx) const {
            return MultiQuery->Queries[queryIdx];
        }
        const TQuery& GetQuery(EExpansionType expansion, size_t queryIdx) const {
            return *ExpInfo[expansion].Queries[queryIdx];
        }
        const TQuery& GetOriginalQuery() const {
            return MultiQuery->Queries[GetOriginalQueryIndex()];
        }

        template <typename QuerySetType>
        TBagOfWords* CreateBagOfWords(
            TMemoryPool& pool,
            EExpansionType expansion,
            bool addOriginal) const;

    private:
        using TQueryWeightsByType = NLingBoost::TStaticEnumMap<TWordFreq, TWeightsHolder>;

        struct TQueryInfo {
            size_t Index = 0;
            size_t LocalIndex = 0;
            TQueryWeightsByType* MainWeightsByType = nullptr;
            TQueryWeightsByType* ExactWeightsByType = nullptr;
        };

        struct TExpansionInfo {
            size_t NumQueries = 0;
            size_t NumWords = 0;
            TPoolPodHolder<const TQuery*> Queries;
            TPoolPodHolder<const TQueryInfo*> Infos;
        };

        using TExpansionsMap = TPoolableCompactEnumMap<TExpansion, TExpansionInfo>;

    private:
        void FillMultiQueryInfo(TMemoryPool& pool,
            const TExpansionRemap& expansionRemap);
        void FillQueryInfo(TMemoryPool& pool, const TQuery& query, TQueryInfo& info);

    private:
        const TMultiQuery* MultiQuery = nullptr;
        TPoolPodHolder<TQueryInfo> QueryInfo;
        TExpansionsMap ExpInfo;
        TMaybe<size_t> OriginalQueryIndex;
        size_t NumBlocks = 0;
    };

    template <typename QuerySetType>
    inline TBagOfWords* TQueriesHelper::CreateBagOfWords(TMemoryPool& pool,
        EExpansionType expansion,
        bool addOriginal) const
    {
        const size_t maxQueryId = Max<size_t>(MultiQuery->Queries.size(), 1) - 1;
        const size_t numQueries = GetNumQueries(expansion)
            + (addOriginal ? 1UL : 0UL);
        const size_t numWords = GetNumWords(expansion)
            + (addOriginal ? GetNumWords(TExpansion::OriginalRequest) : 0UL);

        auto bow = pool.New<TBagOfWords>(pool, maxQueryId, numQueries, numWords);
        for (size_t queryIdx : xrange(MultiQuery->Queries.size())) {
            auto& query = MultiQuery->Queries[queryIdx];
            const bool isOriginal = query.IsOriginal();
            if (expansion == query.ExpansionType
                || (addOriginal && isOriginal))
            {
                float maxValue = -1.0f;

                for (const auto& value : query.Values) {
                    if (isOriginal || QuerySetType::Has(value.Type)) {
                        maxValue = Max(value.Value, maxValue);
                    }
                }

                if (maxValue >= 0.0f) {
                    bow->AddQuery(queryIdx, maxValue,
                        GetMainWeights(queryIdx), GetExactWeights(queryIdx),
                        TExpansion::OriginalRequest == query.ExpansionType);
                }
            }
        }
        bow->Finish();

        return bow;
    }
} // NCore
} // NTextMachine
