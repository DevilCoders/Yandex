#pragma once
#include <kernel/embeddings_info/embedding.h>
#include <kernel/embeddings_info/embedding.h_serialized.h>
#include <util/generic/cast.h>
#include <util/generic/vector.h>
#include <util/ysaveload.h>
#include <array>

namespace NDocDssmEmbeddings {
    struct TEmbedding {
        ui32 Version = 0;
        TVector<float> Coordinates;

        TEmbedding();
        TEmbedding(size_t size);
        TEmbedding(const TVector<float>& coordinates, NDssmApplier::EDssmDocEmbedding embeddingId);
        TEmbedding(const TString& serialized, NDssmApplier::EDssmDocEmbedding embeddingId);

        size_t Size() const;
        TString Serialize(NDssmApplier::EDssmDocEmbeddingType type) const;

        Y_SAVELOAD_DEFINE(Version, Coordinates);
    };

    struct TDocumentEmbeddings {
        std::array<TEmbedding, GetEnumItemsCount<NDssmApplier::EDssmDocEmbedding>()> Embeddings;

        TEmbedding& operator[](NDssmApplier::EDssmDocEmbedding index);
        TEmbedding at(NDssmApplier::EDssmDocEmbedding index) const;
    };
}
