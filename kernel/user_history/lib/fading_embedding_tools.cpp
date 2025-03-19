#include "fading_embedding_tools.h"

#include <kernel/dssm_applier/utils/utils.h>
#include <kernel/embeddings_info/lib/tools.h>
#include <kernel/user_history/user_history.h>

#include <library/cpp/dot_product/dot_product.h>

#include <util/datetime/base.h>
#include <util/generic/ymath.h>
#include <util/string/builder.h>


namespace NPersonalization {
    size_t GetEmbeddingLength(const NEmbedding::TEmbedding& emb) {
        switch (emb.GetFormat()) {
            case NEmbedding::TEmbedding_EFormat::TEmbedding_EFormat_Compressed:
                return emb.GetEmbedding().size();
                break;
            case NEmbedding::TEmbedding_EFormat::TEmbedding_EFormat_Decompressed:
                return emb.GetEmbedding().size() / sizeof(float);
                break;
            default:
                ythrow yexception() << "wrong embedding format";
        }
    }

    float DaysToFadingCoef(const float coefDays) {
        return 1.0f / (coefDays * TInstant::Days(1).Seconds());
    }

    float CalcFading(const float fadingCoef, const ui64 deltaTimestamp) {
        return pow(0.01f, fadingCoef * deltaTimestamp);
    }

    NProto::TFadingEmbedding CreateEmptyFadingEmbedding(
        const float fadingCoefDays,
        const NProto::EModels model,
        const ui64 dwelltimeThreshold,
        const bool lessThanThreshold,
        const NProto::EEmbeddingType embeddingType
    ) {
        NProto::TFadingEmbedding fadingEmb;
        fadingEmb.SetFadingCoefDays(fadingCoefDays);
        fadingEmb.SetLastUpdTimestamp(0);

        auto optsPtr = fadingEmb.MutableOptions();
        optsPtr->SetModel(model);
        optsPtr->SetDwelltimeThreshold(dwelltimeThreshold);
        optsPtr->SetLessThanThreshold(lessThanThreshold);
        optsPtr->SetEmbeddingType(embeddingType);

        auto mutableEmb = fadingEmb.MutableEmbedding();
        NEmbeddingTools::Init(*mutableEmb, NEmbedding::TEmbedding_EFormat::TEmbedding_EFormat_Decompressed);
        mutableEmb->SetFirstVersion(0);
        mutableEmb->SetLastVersion(0);

        return fadingEmb;
    }

    void UpdateFadingEmbedding(NProto::TFadingEmbedding& fadingEmbedding, const TVector<TArrayRef<float>>& embeddings, const TVector<ui64>& ts) {
        Y_ENSURE(embeddings.size() == ts.size());

        auto mutableEmb = fadingEmbedding.MutableEmbedding();
        if (embeddings.empty()) {
            return;
        }

        if (mutableEmb->GetEmbedding().empty()) {
            NEmbeddingTools::SetVector(TVector<float>(embeddings[0].size(), 0.f), *mutableEmb);
        }

        const auto mutableEmbLength = GetEmbeddingLength(*mutableEmb);
        for (const auto& emb : embeddings) {
            const auto embSize = emb.size();
            Y_ENSURE(
                embSize == mutableEmbLength,
                TStringBuilder() << "fadingEmbedding " << mutableEmbLength << " != " << "embedding " << embSize
            );
        }

        const ui64 newTs = Max(*MaxElement(ts.begin(), ts.end()), fadingEmbedding.GetLastUpdTimestamp());
        const float fadingCoef = DaysToFadingCoef(fadingEmbedding.GetFadingCoefDays());
        // back-compatibility
        float oldNorm = 1.f;
        if (mutableEmb->GetFormat() == NEmbedding::TEmbedding_EFormat::TEmbedding_EFormat_Decompressed &&
            !FuzzyEquals(1.f + mutableEmb->GetMultipler(), 1.f))
        {
            oldNorm = mutableEmb->GetMultipler();
        }
        const float oldWeight = oldNorm * CalcFading(fadingCoef, newTs - fadingEmbedding.GetLastUpdTimestamp());
        TVector<float> decompressedEmb = NEmbeddingTools::GetVector(*mutableEmb);
        for (auto& elem : decompressedEmb) {
            elem *= oldWeight;
        }
        for (ui32 i = 0; i < embeddings.size(); ++i) {
            const float newWeight = CalcFading(fadingCoef, newTs - ts[i]);
            for (ui32 j = 0; j < decompressedEmb.size(); ++j) {
                decompressedEmb[j] += embeddings[i][j] * newWeight;
            }
        }
        NEmbeddingTools::SetVector(decompressedEmb, *mutableEmb);
        fadingEmbedding.SetLastUpdTimestamp(newTs);
    }
};
