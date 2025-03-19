#pragma once

#include <kernel/embeddings_info/proto/embedding.pb.h>

#include <util/generic/vector.h>

namespace NEmbeddingTools {
    NEmbedding::TEmbedding CreateEmptyEmbedding(const NEmbedding::TEmbedding_EFormat compression);
    void Init(NEmbedding::TEmbedding& embedding, const NEmbedding::TEmbedding_EFormat compression);

    TVector<float> GetVector(const NEmbedding::TEmbedding& emb);
    void SetVector(const TVector<float>& data, NEmbedding::TEmbedding& emb);

    void Compress(NEmbedding::TEmbedding& emb);
    void Decompress(NEmbedding::TEmbedding& emb);
}
