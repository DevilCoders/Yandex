#pragma once

#include <kernel/user_history/proto/fading_embedding.pb.h>

#include <kernel/dssm_applier/begemot/production_data.h>
#include <kernel/dssm_applier/decompression/dssm_model_decompression.h>

namespace NPersonalization {
    struct TUserHistoryRecord;
    namespace NProto {
        class TUserRecordsDescription;
    }

    bool CheckEmbeddingOpts(
        const NProto::TEmbeddingOptions& opts,
        const NProto::EModels model,
        const NProto::EEmbeddingType type,
        const ui64 dwt,
        const bool lessThanThreshold);

    bool CheckFadingEmbeddingOpts(
        const NProto::TFadingEmbedding& fe,
        const NProto::EModels model,
        const NProto::EEmbeddingType type,
        const ui64 dwt,
        const float fadingCoefDays,
        const bool lessThanThreshold);

    bool FadingEmbeddingsOfTheSameType(const NProto::TFadingEmbedding& f1, const NProto::TFadingEmbedding& f2);

    NNeuralNetApplier::EDssmModel ToDssmModel(const NPersonalization::NProto::EModels model);

    TMaybe<NPersonalization::NProto::EModels> ToProtoEnum(const NDssmApplier::EDssmModelType model);

    TMaybe<NDssmApplier::EDssmModelType> ToDssmModelType(const NProto::EModels model);

    void GetQueryEmbedding(const TString& relevParamValue, const NNeuralNetApplier::EDssmModel model, TVector<float>& result);

    bool CompareUserRecordsDescriptions(const NProto::TUserRecordsDescription& lhs, const NProto::TUserRecordsDescription& rhs, bool ignoreMaxRecords = false);

    void CalcHashes(TUserHistoryRecord& record);
}
