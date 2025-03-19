#include "dssm_embeddings.h"

#include <kernel/embeddings_info/embedding_traits.h>

#include <kernel/dssm_applier/begemot/production_data.h>
#include <kernel/dssm_applier/decompression/decompression.h>
#include <kernel/dssm_applier/utils/utils.h>
#include <kernel/nn_ops/boosting_compression.h>

#include <util/generic/cast.h>
#include <util/generic/maybe.h>
#include <util/stream/mem.h>
#include <util/stream/str.h>

namespace {
    template <typename TNum>
    TString SaveVector(const TVector<TNum>& data) {
        if (data.empty()) {
            return {};
        }
        TStringStream out;
        SaveArray(&out, data.data(), data.size());
        return out.Str();
    }

    template <typename TNum>
    TVector<TNum> LoadVector(const TString& serialized) {
        if (serialized.empty()) {
            return {};
        }
        Y_ASSERT(serialized.size() % sizeof(TNum) == 0);
        TStringInput in(serialized);
        TVector<TNum> data(serialized.size() / sizeof(TNum));
        LoadArray(&in, data.data(), data.size());
        return data;
    }
}

namespace NDocDssmEmbeddings {
    using NDssmApplier::EDssmDocEmbeddingType;
    using NDssmApplier::NUtils::TFloat2UI8Compressor;

    TEmbedding::TEmbedding() = default;

    TEmbedding::TEmbedding(size_t size)
        : Coordinates(size, 0.f)
    {
    }

    TEmbedding::TEmbedding(const TVector<float>& coordinates, NDssmApplier::EDssmDocEmbedding embeddingId)
        : Coordinates(coordinates)
    {
        const NDssmApplier::TDocEmbeddingTransferTraits& transferTraits = NDssmApplier::DocEmbeddingsTransferTraits.at(ToUnderlying(embeddingId));
        const size_t expectedEmbedSize = transferTraits.RequiredEmbeddingSize;

        Y_ENSURE(Coordinates.size() == expectedEmbedSize, TStringBuilder() << "Actual size (" << Coordinates.size() << ") of given " << ToString(transferTraits.Type) << " embedding doesn't match to expectedEmbedSize (" << expectedEmbedSize << ')');
    }

    TEmbedding::TEmbedding(const TString& serialized, NDssmApplier::EDssmDocEmbedding embeddingId) {
        const NDssmApplier::TDocEmbeddingTransferTraits& transferTraits = NDssmApplier::DocEmbeddingsTransferTraits.at(ToUnderlying(embeddingId));
        const NDssmApplier::EDssmDocEmbeddingType type = transferTraits.Type;
        const size_t expectedEmbedSize = transferTraits.RequiredEmbeddingSize;

        switch (type) {
            case EDssmDocEmbeddingType::Uncompressed: {
                TMemoryInput emb(serialized);
                Load(&emb);
                break;
            }
            case EDssmDocEmbeddingType::Compressed: {
                TVector<ui8> compressedCoordinates;
                TMemoryInput emb(serialized);
                LoadMany(&emb, Version, compressedCoordinates);
                Coordinates = TFloat2UI8Compressor::Decompress(compressedCoordinates);
                if (!NNeuralNetApplier::TryNormalize(Coordinates)) {
                    Coordinates.clear();
                }
                break;
            }
            case EDssmDocEmbeddingType::CompressedWithWeights: {
                TVector<float> embed;
                TVector<float> unusedWeights;
                TMaybe<float> normSquared;
                NNeuralNetOps::Decompress(TArrayRef<const ui8>(reinterpret_cast<const ui8*>(serialized.data()), serialized.size()), embed, unusedWeights, normSquared, expectedEmbedSize);

                const bool normProblems = !normSquared.Defined() || !NNeuralNetApplier::TryNormalize(embed, normSquared.GetRef());
                if (Y_UNLIKELY(normProblems)) {
                    Y_ENSURE(false, "Embedding normalization failed");
                    break;
                }
                Y_ENSURE(NNeuralNetApplier::IsNormalized(embed));
                Coordinates = std::move(embed);
                break;
            }
            case EDssmDocEmbeddingType::CompressedLogDwellTimeL2:
                Coordinates = NDssmApplier::Decompress(serialized, NDssmApplier::EDssmModelType::LogDwellTimeBigrams);
                // We have guarantee that embedding is normalized
                break;
            case EDssmDocEmbeddingType::CompressedPantherTerms:
                Coordinates = NDssmApplier::Decompress(serialized, NDssmApplier::EDssmModelType::PantherTerms);
                if (!NNeuralNetApplier::TryNormalize(Coordinates)) {
                    Coordinates.clear();
                }
                break;
            case EDssmDocEmbeddingType::CompressedUserRecDssmSpyTitleDomain:
                Coordinates = NDssmApplier::Decompress(serialized, NDssmApplier::EDssmModelType::RecDssmSpyTitleDomainUser);
                break;
            case EDssmDocEmbeddingType::CompressedUrlRecDssmSpyTitleDomain:
                Coordinates = NDssmApplier::Decompress(serialized, NDssmApplier::EDssmModelType::RecDssmSpyTitleDomainUrl);
                break;
            case EDssmDocEmbeddingType::UncompressedCFSharp:
                Coordinates = LoadVector<float>(serialized);
                break;
        }
        if (Coordinates.size() != expectedEmbedSize) {
            Coordinates.clear();
        }
    }

    size_t TEmbedding::Size() const {
        return Coordinates.size();
    }

    TString TEmbedding::Serialize(NDssmApplier::EDssmDocEmbeddingType type) const {
        switch (type) {
            case EDssmDocEmbeddingType::Uncompressed: {
                TStringStream serialized;
                Save(&serialized);
                return serialized.Str();
            }
            case EDssmDocEmbeddingType::Compressed: {
                TStringStream serialized;
                SaveMany(&serialized, Version, TFloat2UI8Compressor::Compress(Coordinates));
                return serialized.Str();
            }
            case EDssmDocEmbeddingType::CompressedUserRecDssmSpyTitleDomain:
                return SaveVector(NDssmApplier::Compress(Coordinates, NDssmApplier::EDssmModelType::RecDssmSpyTitleDomainUser));
            case EDssmDocEmbeddingType::CompressedUrlRecDssmSpyTitleDomain:
                return SaveVector(NDssmApplier::Compress(Coordinates, NDssmApplier::EDssmModelType::RecDssmSpyTitleDomainUrl));
            case EDssmDocEmbeddingType::UncompressedCFSharp:
                return SaveVector(Coordinates);
            default:
                Y_ASSERT(false);
        }

        return TString();
    }

    TEmbedding& TDocumentEmbeddings::operator[](NDssmApplier::EDssmDocEmbedding index) {
        return Embeddings[ToUnderlying(index)];
    }

    TEmbedding TDocumentEmbeddings::at(NDssmApplier::EDssmDocEmbedding index) const {
        return Embeddings.at(ToUnderlying(index));
    }
}
