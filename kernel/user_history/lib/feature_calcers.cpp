#include "feature_calcers.h"

#include "embedding_tools.h"
#include "fading_embedding_tools.h"

#include <kernel/user_history/proto/user_history.pb.h>
#include <kernel/user_history/lib/calcer_codegen.h>

#include <util/generic/ymath.h>

namespace {
    float CalcDeltaTimestampFeature(
        const float norm,
        const float fadingCoefDays,
        const ui64 lastUpdTimestamp,
        const ui64 requestTimestamp)
    {
        if (requestTimestamp > lastUpdTimestamp && !FuzzyEquals(1.f + fadingCoefDays, 1.f)) {
            const float fadingCoef = NPersonalization::DaysToFadingCoef(fadingCoefDays);
            return norm * NPersonalization::CalcFading(fadingCoef, requestTimestamp - lastUpdTimestamp);
        }
        return 0.f;
    }
}

namespace NPersonalization {
    void TFadingEmbeddingsDeltaTimestampCalcer::CalcFeatures(
        const TVector<float>& norms,
        const TVector<float>& fadingCoefDays,
        const TVector<ui64>& lastUpdTimestamps,
        const ui64 requestTimestamp,
        TArrayRef<float> features) const
    {
        Y_ENSURE(norms.size() == fadingCoefDays.size() && fadingCoefDays.size() == lastUpdTimestamps.size());
        Y_ENSURE(features.size() == norms.size());
        for (ui64 i = 0; i < norms.size(); ++i) {
            features[i] = CalcDeltaTimestampFeature(norms[i], fadingCoefDays[i], lastUpdTimestamps[i], requestTimestamp);
        }
    }

    void TFadingEmbeddingsDeltaTimestampCalcer::CalcFeatures(
        const NProtoBuf::RepeatedPtrField<NProto::TFadingEmbedding>& fadingEmbeddings,
        const ui64 requestTimestamp,
        TArrayRef<float> features) const
    {
        Y_ENSURE(static_cast<size_t>(fadingEmbeddings.size()) == features.size());
        for (int i = 0; i < fadingEmbeddings.size(); ++i) {
            features[i] = CalcDeltaTimestampFeature(
                fadingEmbeddings[i].GetEmbedding().GetNorm(),
                fadingEmbeddings[i].GetFadingCoefDays(),
                fadingEmbeddings[i].GetLastUpdTimestamp(),
                requestTimestamp);
        }
    }

    void Normalize(EFeatureNormalizationType type, const TArrayRef<float> from, TArrayRef<float> to) {
        switch (type) {
            case EFeatureNormalizationType::WithoutNormalization:
                break;
            case EFeatureNormalizationType::OneMinusExp02:
                ::Transform(from.begin(), from.end(), to.begin(), [] (const float x) {
                    return 1.f - ::Exp2f(-0.2f * x);
                });
                break;
        }
    }


    EClusterizationOptionsType GetClusterizationOptionsType(const NPersonalization::NProto::TEmbeddingOptions& opts) {
        using NPersonalization::CheckEmbeddingOpts;
        using namespace NPersonalization::NProto;
        if (CheckEmbeddingOpts(opts, LogDwelltimeBigrams, TitleEmbedding, 120, true)) {
            return CT_LOG_DT_BIGRAMS_SHORT_CLICKS;
        } else if (CheckEmbeddingOpts(opts, LogDwelltimeBigrams, TitleEmbedding, 120, false)) {
            return CT_LOG_DT_BIGRAMS_LONG_CLICKS;
        } else if (CheckEmbeddingOpts(opts, LogDtBigramsQueryPart, QueryEmbedding, 0, false)) {
            return CT_LOG_DT_BIGRAMS_QUERIES;
        }
        return CT_IGNORE;
    }

    EFilteredHistoryType GetFilteredHistoryType(const NPersonalization::NProto::TUserRecordsDescription& description) {
        using namespace NPersonalization::NProto;
        const auto options = description.GetOptions();
        if (description.GetMaxRecords() == 50 &&
                NPersonalization::CheckEmbeddingOpts(options, EModels::LogDwelltimeBigrams, EEmbeddingType::TitleEmbedding, 120, false)) {
            return FHT_LOG_DT_BIGRAMS_LONG_D120_CLICKS;
        } else if (description.GetMaxRecords() == 25 &&
                NPersonalization::CheckEmbeddingOpts(options, EModels::LogDwelltimeBigrams, EEmbeddingType::TitleEmbedding, 0, false)) {
            return FHT_LOG_DT_BIGRAMS_ALL_CLICKS;
        } else if (description.GetMaxRecords() == 50 &&
                NPersonalization::CheckEmbeddingOpts(options, EModels::LogDtBigramsQueryPart, EEmbeddingType::QueryEmbedding, 0, false)) {
            return FHT_LOG_DT_BIGRAMS_QUERIES;
        }
        return FHT_IGNORE;
    }

    template <class TBooster, class TCalcer>
    class TFeatureHelper : public IFeatureHelper {
    private:
        TCalcer Calcer;
        TBooster Booster;

    public:
        void CalcFeatures(
            const TVector<float>& embedding,
            const TVector<TVector<float>>& userEmbeddings,
            const TVector<TVector<float>>& weights,
            const TArrayRef<float> result,
            const TVector<float>& thresholdValues) const override
        {
            Calcer.CalcFeatures(embedding, userEmbeddings, weights, result, thresholdValues);
        }

        size_t GetNFeatures() const override {
            return Booster.GetNFeatures();
        }
    };

    THolder<IFeatureHelper> CreateQueryFeatureHelper(const EClusterizationOptionsType type) {
        using namespace NVectorMachine;

        switch(type) {
            case CT_IGNORE:
                return nullptr;
            case CT_LOG_DT_BIGRAMS_SHORT_CLICKS:
                return THolder(new TFeatureHelper<TShortClusteredEmbeddingsQueryFeaturesBooster, TShortClusteredEmbeddingsQueryFeaturesCalcer>());
            case CT_LOG_DT_BIGRAMS_LONG_CLICKS:
                return THolder(new TFeatureHelper<TLongClusteredEmbeddingsQueryFeaturesBooster, TLongClusteredEmbeddingsQueryFeaturesCalcer>());
            case CT_LOG_DT_BIGRAMS_QUERIES:
                return THolder(new TFeatureHelper<TQueryClusteredEmbeddingsQueryFeaturesBooster, TQueryClusteredEmbeddingsQueryFeaturesCalcer>());
        }
    }

    THolder<IFeatureHelper> CreateDocFeatureHelper(const EClusterizationOptionsType type) {
        using namespace NVectorMachine;

        switch(type) {
            case CT_IGNORE:
                return nullptr;
            case CT_LOG_DT_BIGRAMS_SHORT_CLICKS:
                return THolder(new TFeatureHelper<TShortClusteredEmbeddingsDocFeaturesBooster, TShortClusteredEmbeddingsDocFeaturesCalcer>());
            case CT_LOG_DT_BIGRAMS_LONG_CLICKS:
                return THolder(new TFeatureHelper<TLongClusteredEmbeddingsDocFeaturesBooster, TLongClusteredEmbeddingsDocFeaturesCalcer>());
            case CT_LOG_DT_BIGRAMS_QUERIES:
                return THolder(new TFeatureHelper<TQueryClusteredEmbeddingsDocFeaturesBooster, TQueryClusteredEmbeddingsDocFeaturesCalcer>());
        }
    }

    THolder<IFeatureHelper> CreateFadingEmbeddingsHelper(const NProto::EModels model) {
        using namespace NVectorMachine;

        switch (model) {
            case NProto::LogDwelltimeBigrams:
                return THolder(new TFeatureHelper<TFadingEmbeddingsLogDtBigramsFeaturesBooster, TFadingEmbeddingsLogDtBigramsFeaturesCalcer>());
            case NProto::LogDtBigramsQueryPart:
                return THolder(new TFeatureHelper<TFadingEmbeddingsLogDtBigramsQueryPartFeaturesBooster, TFadingEmbeddingsLogDtBigramsQueryPartFeaturesCalcer>());
            default:
                return nullptr;
        }
    }

    THolder<IFeatureHelper> CreateQueryFeatureHelper(const EFilteredHistoryType type) {
        using namespace NVectorMachine;

        switch(type) {
            case FHT_IGNORE:
                return nullptr;
            case FHT_LOG_DT_BIGRAMS_ALL_CLICKS:
                return THolder(new TFeatureHelper<TUserRecordsQueryAllClicksBooster, TUserRecordsQueryAllClicksCalcer>());
            case FHT_LOG_DT_BIGRAMS_LONG_D120_CLICKS:
                return THolder(new TFeatureHelper<TUserRecordsQueryLongD120ClicksBooster, TUserRecordsQueryLongD120ClicksCalcer>());
            case FHT_LOG_DT_BIGRAMS_QUERIES:
                return THolder(new TFeatureHelper<TQueryHistoryQueryFeaturesBooster, TQueryHistoryQueryFeaturesCalcer>());
        }
    }

    THolder<IFeatureHelper> CreateDocFeatureHelper(const EFilteredHistoryType type) {
        using namespace NVectorMachine;

        switch(type) {
            case FHT_IGNORE:
                return nullptr;
            case FHT_LOG_DT_BIGRAMS_ALL_CLICKS:
                return THolder(new TFeatureHelper<TUserRecordsDocAllClicksBooster, TUserRecordsDocAllClicksCalcer>());
            case FHT_LOG_DT_BIGRAMS_LONG_D120_CLICKS:
                return THolder(new TFeatureHelper<TUserRecordsDocLongD120ClicksBooster, TUserRecordsDocLongD120ClicksCalcer>());
            case FHT_LOG_DT_BIGRAMS_QUERIES:
                return THolder(new TFeatureHelper<TQueryHistoryDocFeaturesBooster, TQueryHistoryDocFeaturesCalcer>());
        }
    }
}
