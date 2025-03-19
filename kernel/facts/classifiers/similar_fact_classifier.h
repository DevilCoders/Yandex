#pragma once

#include "classifier_base.h"
#include "classifier_data.h"
#include "similar_fact.h"

#include <kernel/facts/factors_info/factor_names.h>
#include <kernel/facts/features_calculator/query_features.h>

namespace NFactClassifiers {
    const size_t SIMILAR_FACTS_VERSION = 1;
    const TString SIMILAR_FACTS_CONFIG_NAME = "similar_facts.config";

    class TSimilarFactClassifier : public TClassifierBase {
    public:
        TSimilarFactClassifier(const TClassifierData& data, const TString& query, const TUtf16String& normedQuery)
            : TClassifierBase(data)
            , Query(data.FeaturesData, query, normedQuery) {}

        NUnstructuredFeatures::TFactFactorStorage CalculateFeatures(const TSimilarFact& fact, TQueryFactorStorageCache* factorStorageCache = nullptr) const override;

    private:
        NUnstructuredFeatures::TQueryCalculator Query;
    };
}

