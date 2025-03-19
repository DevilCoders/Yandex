#pragma once

#include "classifier_base.h"
#include "classifier_data.h"
#include "similar_fact.h"

#include <kernel/facts/factors_info/factor_names.h>
#include <kernel/facts/features_calculator/query_features.h>

namespace NFactClassifiers {
    const size_t ONLINE_ALIASES_VERSION = 3;
    const TString ONLINE_ALIASES_CONFIG_NAME = "online_aliases.config";

    class TOnlineAliasClassifier : public TClassifierBase {
    public:
        TOnlineAliasClassifier(const TClassifierData& data, const TString& query, const TUtf16String& normedQuery,
                         TVector<float>&& queryEmbed, const TString& service)
            : TClassifierBase(data)
            , Query(data.FeaturesData, query, normedQuery, std::move(queryEmbed))
            , Service(service)
        {}

        TOnlineAliasClassifier(const TClassifierData& data, const TString& query, const TUtf16String& normedQuery,
                         const TString& service = "")
            : TClassifierBase(data)
            , Query(data.FeaturesData, query, normedQuery)
            , Service(service) {}

        TOnlineAliasClassifier(const TClassifierData& data, const TString& query, const TString& service = "")
            : TClassifierBase(data)
            , Query(data.FeaturesData, query)
            , Service(service) {}

        NUnstructuredFeatures::TFactFactorStorage CalculateFeatures(const TSimilarFact& fact, TQueryFactorStorageCache* factorStorageCache = nullptr) const override;

        const TVector<float>& GetQueryEmbedding() const {
            return Query.GetQueryEmbedding();
        }

    private:
        NUnstructuredFeatures::TQueryCalculator Query;
        const TString& Service;
    };
}
