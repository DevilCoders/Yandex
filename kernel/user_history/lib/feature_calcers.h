#pragma once

#include <kernel/user_history/proto/embedding_types.pb.h>

#include <util/generic/fwd.h>

namespace NPersonalization {
    namespace NProto {
        class TEmbeddingOptions;
        class TFadingEmbedding;
        class TUserRecordsDescription;
    }

    enum class EFeatureNormalizationType {
        WithoutNormalization,
        OneMinusExp02,
    };

    void Normalize(EFeatureNormalizationType type, TArrayRef<float> from, TArrayRef<float> to);

    class TFadingEmbeddingsDeltaTimestampCalcer {
    public:
        void CalcFeatures(
            const TVector<float>& norms,
            const TVector<float>& fadingCoefDays,
            const TVector<ui64>& lastUpdTimestamps,
            const ui64 requestTimestamp,
            TArrayRef<float> features) const;

        void CalcFeatures(
            const NProtoBuf::RepeatedPtrField<NProto::TFadingEmbedding>& fadingEmbeddings,
            const ui64 requestTimestamp,
            TArrayRef<float> features) const;
    };

    enum EClusterizationOptionsType {
        CT_IGNORE,
        CT_LOG_DT_BIGRAMS_SHORT_CLICKS,
        CT_LOG_DT_BIGRAMS_LONG_CLICKS,
        CT_LOG_DT_BIGRAMS_QUERIES,
    };

    enum EFilteredHistoryType {
        FHT_IGNORE,
        FHT_LOG_DT_BIGRAMS_ALL_CLICKS,
        FHT_LOG_DT_BIGRAMS_LONG_D120_CLICKS,
        FHT_LOG_DT_BIGRAMS_QUERIES,
    };

    EClusterizationOptionsType GetClusterizationOptionsType(const NPersonalization::NProto::TEmbeddingOptions& opts);

    EFilteredHistoryType GetFilteredHistoryType(const NPersonalization::NProto::TUserRecordsDescription& description);

    class IFeatureHelper {
    public:
        virtual ~IFeatureHelper() {
        }

        virtual void CalcFeatures(
            const TVector<float>& embedding,
            const TVector<TVector<float>>& userEmbeddings,
            const TVector<TVector<float>>& weights,
            const TArrayRef<float> result,
            const TVector<float>& thresholdValues) const = 0;

        virtual size_t GetNFeatures() const = 0;
    };

    THolder<IFeatureHelper> CreateQueryFeatureHelper(const EClusterizationOptionsType type);
    THolder<IFeatureHelper> CreateDocFeatureHelper(const EClusterizationOptionsType type);

    THolder<IFeatureHelper> CreateQueryFeatureHelper(const EFilteredHistoryType type);
    THolder<IFeatureHelper> CreateDocFeatureHelper(const EFilteredHistoryType type);

    THolder<IFeatureHelper> CreateFadingEmbeddingsHelper(const NProto::EModels model);
}
