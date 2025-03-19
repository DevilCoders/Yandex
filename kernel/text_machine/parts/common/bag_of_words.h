#pragma once

#include "weights.h"
#include <kernel/text_machine/module/save_to_json.h>

#include <util/memory/pool.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    class TBagOfWords
        : public TNonCopyable
        , public NModule::TJsonSerializable
    {
    public:
        enum class EWordWeightType {
            Main = 0,
            Exact = 1
        };

    public:
        TBagOfWords(TMemoryPool& pool,
            TQueryId maxQueryId,
            TQueryId queryCountLimit,
            TQueryWordId wordCountLimit)
        {
            ExternalId2BagQueryId.Init(pool, maxQueryId + 1, EStorageMode::Full);
            ExternalId2BagQueryId.Fill(InvalidQueryId);

            QueryWeights.Init(pool, queryCountLimit, EStorageMode::Empty);
            NormQueryWeights.Init(pool, queryCountLimit, EStorageMode::Empty);
            QuerySizes.Init(pool, queryCountLimit, EStorageMode::Empty);
            QueryFirstWordIndexes.Init(pool, queryCountLimit, EStorageMode::Empty);

            MainWordWeights.Init(pool, wordCountLimit, EStorageMode::Empty);
            ExactWordWeights.Init(pool, wordCountLimit, EStorageMode::Empty);

            AllMainWordWeights.Init(pool, wordCountLimit, EStorageMode::Empty);
            AllExactWordWeights.Init(pool, wordCountLimit, EStorageMode::Empty);
        }

        void AddQuery(TQueryId externalQueryId,
            float queryWeight,
            const TWeights& queryMainWordWeights,
            const TWeights& queryExactWordWeights,
            bool isOriginalQuery)
        {
            Y_ASSERT(queryMainWordWeights.Count() == queryExactWordWeights.Count());
            const TQueryWordId numWords = queryMainWordWeights.Count();
            const TQueryWordId firstWordId = MainWordWeights.Count();

            ExternalId2BagQueryId[externalQueryId] = QueryCount;
            if (isOriginalQuery) {
                OriginalQueryId = QueryCount;
            }
            QueryWeights.Add(queryWeight);
            QuerySizes.Add(numWords);
            QueryFirstWordIndexes.Add(firstWordId);

            NormalizeAndAppendWeights(queryMainWordWeights, MainWordWeights);
            NormalizeAndAppendWeights(queryExactWordWeights, ExactWordWeights);

            WordCount += QuerySizes[QueryCount];
            ++QueryCount;
        }

        void Finish() {
            NormalizeWeights(QueryWeights, NormQueryWeights);
            PrepareAllWordWeights(AllMainWordWeights, EWordWeightType::Main);
            PrepareAllWordWeights(AllExactWordWeights, EWordWeightType::Exact);
        }

        void GetBagIds(TQueryId externalQueryId,
            TQueryWordId queryWordPos,
            TQueryId& bagQueryId,
            TQueryWordId& bagWordId) const
        {
            bagQueryId = GetBagQueryId(externalQueryId);
            Y_ASSERT(queryWordPos < QuerySizes[bagQueryId]);
            bagWordId = QueryFirstWordIndexes[bagQueryId] + queryWordPos;
        }

        TQueryId GetQueryCount() const {
            return QueryCount;
        }

        TQueryId GetOriginalQueryId() const {
            Y_ASSERT(OriginalQueryId != InvalidQueryId);
            return OriginalQueryId;
        }
        float GetQueryWeight(TQueryId bagQueryId) const {
            return QueryWeights[bagQueryId];
        }
        TQueryWordId GetQuerySize(TQueryId bagQueryId) const {
            return QuerySizes[bagQueryId];
        }

        TQueryWordId GetWordCount() const {
            return WordCount;
        }

        //single word weight, weights are normalized per extension
        float GetWordWeight(TQueryWordId bagWordId,
            EWordWeightType type = EWordWeightType::Main) const
        {
            return (type == EWordWeightType::Main)
                ? MainWordWeights[bagWordId]
                : ExactWordWeights[bagWordId];
        }

        //weights are normalized for the whole bag
        float GetBagWordWeight(TQueryWordId bagWordId,
            EWordWeightType type = EWordWeightType::Main) const
        {
            return EWordWeightType::Main == type
                ? AllMainWordWeights[bagWordId]
                : AllExactWordWeights[bagWordId];
        }

        void SaveToJson(NJson::TJsonValue& value) const {
            SAVE_JSON_VAR(value, ExternalId2BagQueryId);
            SAVE_JSON_VAR(value, QueryWeights);
            SAVE_JSON_VAR(value, QueryFirstWordIndexes);
            SAVE_JSON_VAR(value, QuerySizes);
            SAVE_JSON_VAR(value, MainWordWeights);
            SAVE_JSON_VAR(value, ExactWordWeights);
            SAVE_JSON_VAR(value, QueryCount);
            SAVE_JSON_VAR(value, OriginalQueryId);
            SAVE_JSON_VAR(value, WordCount);
        }

    private:
        void PrepareAllWordWeights(TWeights& wordWeights,
            const EWordWeightType type = EWordWeightType::Main) const
        {
            wordWeights.SetTo(WordCount);
            size_t bagWordId = 0;
            for (size_t i : xrange(QueryCount)) {
                TQueryWordId querySize = QuerySizes[i];
                for (TQueryWordId j : xrange(querySize)) {
                    Y_UNUSED(j);
                    wordWeights[bagWordId] = NormQueryWeights[i] * GetWordWeight(bagWordId, type);
                    ++bagWordId;
                }
            }
        }

        void NormalizeWeights(const TWeights& weights,
            TWeights& weightsNorm) const
        {
            weightsNorm.SetTo(weights.Count());
            TWeightsHelper weightsHelper(weightsNorm);
            for (size_t i = 0; i != weights.Count(); ++i) {
                weightsHelper.SetWordWeight(i, weights[i]);
            }
            weightsHelper.Finish();
        }

        void NormalizeAndAppendWeights(const TWeights& weights,
            TWeights& bagWordWeights) const
        {
            TWeights normWeights = bagWordWeights.Append(weights.Count());
            NormalizeWeights(weights, normWeights);
        }

        TQueryId GetBagQueryId(TQueryId externalQueryId) const {
            Y_ASSERT(ExternalId2BagQueryId[externalQueryId] != InvalidQueryId);
            return ExternalId2BagQueryId[externalQueryId];
        }

        TPoolPodHolder<TQueryId> ExternalId2BagQueryId;
        TWeightsHolder QueryWeights;
        TWeightsHolder NormQueryWeights;
        TPoolPodHolder<TQueryWordId> QueryFirstWordIndexes;
        TPoolPodHolder<TQueryWordId> QuerySizes;

        TWeightsHolder MainWordWeights;
        TWeightsHolder ExactWordWeights;

        TWeightsHolder AllMainWordWeights;
        TWeightsHolder AllExactWordWeights;

        TQueryId QueryCount = 0;
        TQueryId OriginalQueryId = Max<TQueryId>();
        TQueryWordId WordCount = 0;

        const TQueryId InvalidQueryId = Max<TQueryId>();
    };
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
