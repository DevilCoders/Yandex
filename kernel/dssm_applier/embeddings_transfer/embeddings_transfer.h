#pragma once

#include <kernel/dssm_applier/embeddings_transfer/proto/model_embedding.pb.h>

#include <util/generic/vector.h>
#include <util/stream/mem.h>
#include <util/stream/str.h>

namespace NEmbeddingsTransfer {

    struct TEmbedding {
        TString Version;
        TVector<float> Data;

        TEmbedding() = default;

        TEmbedding(TString version, TVector<float> data)
            : Version(std::move(version))
            , Data(std::move(data)) {
        }
    };

    using TEmbeddingData = std::pair<TString, TEmbedding>;

    NProto::ECompressionType GetCompressionType(TStringBuf embeddingName, TStringBuf embeddingVersion);

    void SetEmbeddingData(NProto::TModelEmbedding& modelEmbedding, TStringBuf embeddingName,
        const TEmbedding& rawEmbedding);

    TEmbeddingData GetEmbeddingData(const NProto::TModelEmbedding& modelEmbedding);

    TString CompressEmbedding(TConstArrayRef<float> embeddingData,
        const NProto::ECompressionType compressionType, NProto::TCompressionInfo &info);

    TVector<float> DecompressEmbedding(TStringBuf compressedEmbedding,
        const NProto::ECompressionType compressionType,
        const std::optional<NProto::TCompressionInfo> &compressionInfo = {});

} // namespace NEmbeddingsTransfer
