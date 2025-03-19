#include "embeddings_transfer.h"

#include <kernel/dssm_applier/utils/utils.h>

NEmbeddingsTransfer::NProto::ECompressionType NEmbeddingsTransfer::GetCompressionType(TStringBuf embeddingName, TStringBuf embeddingVersion) {
    Y_UNUSED(embeddingVersion); // Delete this when different versions of some embedding will use different compression types
    if (embeddingName == "query_bert_embed")
        return NEmbeddingsTransfer::NProto::ECompressionType::AUTO_MAX_COORD_RENORM_TO_UI8_WITH_SCALE;
    if (embeddingName == "doc_embedding_dssm_bert_distill_l2")
        return NEmbeddingsTransfer::NProto::ECompressionType::AUTO_MAX_COORD_RENORM_TO_UI8;
    if (embeddingName == "doc_embedding_dssm_navigation_l2")
        return NEmbeddingsTransfer::NProto::ECompressionType::AUTO_MAX_COORD_RENORM_TO_UI8;
    if (embeddingName == "doc_embedding_dssm_sinsig_l2")
        return NEmbeddingsTransfer::NProto::ECompressionType::AUTO_MAX_COORD_RENORM_TO_UI8;
    return NEmbeddingsTransfer::NProto::ECompressionType::AUTO_MAX_COORD_RENORM_TO_UI8;
}

void NEmbeddingsTransfer::SetEmbeddingData(NEmbeddingsTransfer::NProto::TModelEmbedding& modelEmbedding,
    TStringBuf embeddingName, const TEmbedding& rawEmbedding)
{
    NEmbeddingsTransfer::NProto::ECompressionType compressionType = NEmbeddingsTransfer::GetCompressionType(
        embeddingName, rawEmbedding.Version);
    NEmbeddingsTransfer::NProto::TCompressionInfo info;
    modelEmbedding.SetName(TString{embeddingName});
    modelEmbedding.SetVersion(rawEmbedding.Version);
    modelEmbedding.SetCompressionType(compressionType);
    modelEmbedding.SetCompressedData(CompressEmbedding(rawEmbedding.Data, compressionType, info));
    if (compressionType != NEmbeddingsTransfer::NProto::ECompressionType::NO_COMPRESSION &&
        compressionType != NEmbeddingsTransfer::NProto::ECompressionType::AUTO_MAX_COORD_RENORM_TO_UI8)
        *(modelEmbedding.MutableCompressionInfo()) = info;
}

NEmbeddingsTransfer::TEmbeddingData NEmbeddingsTransfer::GetEmbeddingData(
    const NEmbeddingsTransfer::NProto::TModelEmbedding& modelEmbedding)
{
    std::optional<NEmbeddingsTransfer::NProto::TCompressionInfo> info;
    if (modelEmbedding.HasCompressionInfo())
        info = modelEmbedding.GetCompressionInfo();
    return {
        modelEmbedding.GetName(),
        TEmbedding(modelEmbedding.GetVersion(), DecompressEmbedding(
            modelEmbedding.GetCompressedData(), modelEmbedding.GetCompressionType(), info))
    };
}

TString NEmbeddingsTransfer::CompressEmbedding(TConstArrayRef<float> embeddingData,
    const NEmbeddingsTransfer::NProto::ECompressionType compressionType,
    NEmbeddingsTransfer::NProto::TCompressionInfo &info)
{
    TStringStream output;

    switch (compressionType) {
        case (NEmbeddingsTransfer::NProto::ECompressionType::NO_COMPRESSION): {
            output.Write(embeddingData.data(), sizeof(float) * embeddingData.size());
            break;
        }
        case (NEmbeddingsTransfer::NProto::ECompressionType::AUTO_MAX_COORD_RENORM_TO_UI8): {
            TVector<ui8> compressedEmbedding = NDssmApplier::NUtils::TFloat2UI8Compressor::Compress(embeddingData);
            output.Write(compressedEmbedding.data(), sizeof(ui8) * compressedEmbedding.size());
            break;
        }
        case (NEmbeddingsTransfer::NProto::ECompressionType::AUTO_MAX_COORD_RENORM_TO_UI8_WITH_SCALE): {
            float scale;
            TVector<ui8> compressedEmbedding = NDssmApplier::NUtils::TFloat2UI8Compressor::Compress(embeddingData, &scale);
            output.Write(compressedEmbedding.data(), sizeof(ui8) * compressedEmbedding.size());
            info.SetScale(scale);
            break;
        }
    }

    return output.Str();
}

TVector<float> NEmbeddingsTransfer::DecompressEmbedding(TStringBuf compressedEmbedding,
    const NEmbeddingsTransfer::NProto::ECompressionType compressionType,
    const std::optional<NEmbeddingsTransfer::NProto::TCompressionInfo> &compressionInfo)
{
    TVector<float> embeddingData;
    TMemoryInput input(compressedEmbedding.data(), compressedEmbedding.size());

    switch (compressionType) {
        case (NEmbeddingsTransfer::NProto::ECompressionType::NO_COMPRESSION): {
            embeddingData.resize(compressedEmbedding.size() / sizeof(float));
            input.Read(embeddingData.data(), compressedEmbedding.size());
            break;
        }
        case (NEmbeddingsTransfer::NProto::ECompressionType::AUTO_MAX_COORD_RENORM_TO_UI8): {
            TVector<ui8> compressedData(compressedEmbedding.size());
            input.Read(compressedData.data(), compressedEmbedding.size());
            embeddingData = NDssmApplier::NUtils::TFloat2UI8Compressor::Decompress(compressedData);
            break;
        }
        case (NEmbeddingsTransfer::NProto::ECompressionType::AUTO_MAX_COORD_RENORM_TO_UI8_WITH_SCALE): {
            float scale = compressionInfo->GetScale();
            TVector<ui8> compressedData(compressedEmbedding.size());
            input.Read(compressedData.data(), compressedEmbedding.size());
            embeddingData = NDssmApplier::NUtils::TFloat2UI8Compressor::Decompress(compressedData);
            std::for_each(embeddingData.begin(), embeddingData.end(), [scale](decltype(embeddingData)::value_type &i){i *= scale;});
            break;
        }
        default: {
            Y_ENSURE(false, "Can't decompress embedding with compression type" << compressionType);
        }
    }

    return embeddingData;
}
