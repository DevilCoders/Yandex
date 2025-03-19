#include "queries_helper.h"

namespace NTextMachine {
namespace NCore {
    void TQueriesHelper::FillMultiQueryInfo(TMemoryPool& pool,
        const TExpansionRemap& expansionRemap)
    {
        Y_ASSERT(!MultiQuery->Queries.empty());

        ExpInfo = TExpansionsMap(pool, expansionRemap.View());

        QueryInfo.Init(pool, MultiQuery->Queries.size(), EStorageMode::Full);
        QueryInfo.Fill(TQueryInfo());

        for (size_t queryIdx : xrange(MultiQuery->Queries.size())) {
            auto& query = MultiQuery->Queries[queryIdx];
            auto& info = QueryInfo[queryIdx];

            bool isOriginal = TExpansion::OriginalRequest == query.ExpansionType;

            info.Index = queryIdx;
            if (!expansionRemap.HasKey(query.ExpansionType)) {
                info.LocalIndex = Max<size_t>(); // Signal that query was ignored
                continue;
            }
            FillQueryInfo(pool, query, info);

            auto& expInfo = ExpInfo[query.ExpansionType];

            info.LocalIndex = expInfo.NumQueries;
            expInfo.NumQueries += 1;
            expInfo.NumWords += query.Words.size();

            if (isOriginal) {
                Y_ASSERT(!OriginalQueryIndex.Defined());
                OriginalQueryIndex = queryIdx;
            }
        }

        for (EExpansionType expType : ExpInfo.GetKeys()) {
            auto& expInfo = ExpInfo[expType];
            expInfo.Queries.Init(pool, expInfo.NumQueries, EStorageMode::Empty);
            expInfo.Infos.Init(pool, expInfo.NumQueries, EStorageMode::Empty);
        }

        for (size_t queryIdx : xrange(MultiQuery->Queries.size())) {
            if (Max<size_t>() == QueryInfo[queryIdx].LocalIndex) {
                continue; // this request was previously ignored
            }

            auto& query = MultiQuery->Queries[queryIdx];
            auto& expInfo = ExpInfo[query.ExpansionType];

            Y_ASSERT(QueryInfo[queryIdx].LocalIndex == expInfo.Queries.Count());

            expInfo.Queries.Add(&query);
            expInfo.Infos.Add(&QueryInfo[queryIdx]);
        }

        NumBlocks += 1;
    }

    void TQueriesHelper::FillQueryInfo(TMemoryPool& pool,
        const TQuery& query,
        TQueriesHelper::TQueryInfo& info)
    {
        const size_t numWords = query.Words.size();
        Y_ASSERT(numWords > 0);

        info.MainWeightsByType = pool.New<TQueryWeightsByType>();
        info.ExactWeightsByType = pool.New<TQueryWeightsByType>();

        for (EWordFreqType freqType : TWordFreq::GetValues()) {
            (*info.MainWeightsByType)[freqType].Init(pool, numWords, EStorageMode::Full);
            (*info.ExactWeightsByType)[freqType].Init(pool, numWords, EStorageMode::Full);

            TWeightsHelper mainHelper((*info.MainWeightsByType)[freqType]);
            TWeightsHelper exactHelper((*info.ExactWeightsByType)[freqType]);

            for (size_t i : xrange(numWords)) {
                mainHelper.SetWordWeight(i, query.Words[i].GetMainWeight(freqType));
                exactHelper.SetWordWeight(i, query.Words[i].GetExactWeight(freqType));

                for (size_t j : xrange(query.Words[i].Forms.size())) {
                    auto& form = *query.Words[i].Forms[j];
                    NumBlocks = Max<size_t>(NumBlocks, form.MatchedBlockId);
                }
            }
            mainHelper.Finish();
            exactHelper.Finish();
        }
    }
} // NCore
} // NTextMachine
