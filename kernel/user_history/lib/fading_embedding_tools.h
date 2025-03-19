#pragma once

#include <kernel/dssm_applier/begemot/production_data.h>
#include <kernel/user_history/proto/fading_embedding.pb.h>

#include <util/generic/vector.h>

namespace NPersonalization {
    struct TUserHistoryRecord;

    size_t GetEmbeddingLength(const NEmbedding::TEmbedding& emb);

    float DaysToFadingCoef(const float coefDays);

    float CalcFading(const float fadingCoef, const ui64 deltaTimestamp);

    TVector<float> DecompressRecordEmbedding(const TString& embedding);

    NProto::TFadingEmbedding CreateEmptyFadingEmbedding(
        const float fadingCoefDays,
        const NProto::EModels model,
        const ui64 dwelltimeThreshold,
        const bool lessThanThreshold,
        const NProto::EEmbeddingType embeddingType
    );

    void UpdateFadingEmbedding(NProto::TFadingEmbedding& fadingEmbedding, const TVector<TArrayRef<float>>& embeddings, const TVector<ui64>& ts);
}
