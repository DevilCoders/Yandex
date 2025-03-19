#include "similar_fact_classifier.h"

#include <kernel/facts/common/normalize_text.h>

using namespace NUnstructuredFeatures;

namespace NFactClassifiers {
    TFactFactorStorage TSimilarFactClassifier::CalculateFeatures(const TSimilarFact& fact, TQueryFactorStorageCache* factorStorageCache) const {
        TFactFactorStorage features;
        TQueryCalculator rhs(Query.GetData(), fact.Query, fact.NormedQuery);

        const auto cachedFactor = (factorStorageCache != nullptr)
                ? MakeMaybe(factorStorageCache->find(fact.Query))
                : Nothing();
        if (cachedFactor && *cachedFactor != factorStorageCache->end()) {
            features = (*cachedFactor)->second;
        } else {
            Query.BuildDssmFeatures(rhs, features);
            Query.BuildNeocortexSimilarityFeatures(rhs, features);
            Query.BuildNeocortexAliasFeatures(rhs, features);
            Query.BuildNeocortexSerpItemFeatures(rhs, features);

            features[FI_ANSWER_LEN] = fact.Answer.size();
            features[FI_QUERY_QUESTION_WORD_ID] = Query.GetQueryQuestionId();
            features[FI_RHS_QUESTION_WORD_ID] = rhs.GetQueryQuestionId();

            Query.BuildEditDistanceFeatures(rhs, features);
            Query.BuildWeightedEditDistanceFeatures(rhs, features);

            Query.BuildWordFeatures(rhs, features);
            Query.BuildNumberInQueryFeatures(rhs, features);
            Query.BuildQueriesLengthsFeatures(rhs, features);
            Query.BuildQuestionsFeatures(rhs, features);

            if (factorStorageCache) {
                factorStorageCache->insert({fact.Query, features});
            }
        }

        TUtf16String answer = NormalizeText(UTF8ToWide<true>(fact.Answer));

        Query.BuildNeocortexFactSnipFeatures(rhs, answer, features);
        Query.BuildWordEmbeddingFeatures(rhs, answer, features);

        return features;
    }
}

