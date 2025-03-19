#pragma once

#include "loc_resolver.h"

#include <kernel/doom/chunked_wad/doc_chunk_mapping_searcher.h>

namespace NDoom {

struct TChunkedFlatLocation: public TErasureBlobLocation {
    ui32 Chunk;
};


class TChunkedFlatLocationResolver {
public:
    TChunkedFlatLocationResolver() = default;

    template<typename... Args>
    TChunkedFlatLocationResolver(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(NDoom::TDocChunkMappingSearcher* chunkResolver, TVector<const TErasureLocationResolver*> locationResolvers) {
        ChunkResolver_ = chunkResolver;
        Resolvers_ = std::move(locationResolvers);
        PartOptimizedResolvers_.clear();
    }

    void Reset(NDoom::TDocChunkMappingSearcher* chunkResolver, TVector<const TPartOptimizedErasureLocationResolver*> locationResolvers) {
        ChunkResolver_ = chunkResolver;
        Resolvers_.clear();
        PartOptimizedResolvers_ = std::move(locationResolvers);
    }

    TMaybe<TChunkedFlatLocation> Resolve(ui32 docId) const {
        if (docId >= ChunkResolver_->Size()) {
            return Nothing();
        }

        TDocChunkMapping chunkMapping = ChunkResolver_->Find(docId);

        TMaybe<TErasureBlobLocation> location;
        if (Resolvers_) {
            Y_VERIFY(chunkMapping.Chunk() < Resolvers_.size());
            location = Resolvers_[chunkMapping.Chunk()]->Resolve(chunkMapping.LocalDocId());
        } else {
            Y_VERIFY(chunkMapping.Chunk() < PartOptimizedResolvers_.size());
            location = PartOptimizedResolvers_[chunkMapping.Chunk()]->Resolve(chunkMapping.LocalDocId());
        }

        if (location) {
            TChunkedFlatLocation result;
            result.Chunk = chunkMapping.Chunk();
            result.Part = location->Part;
            result.Offset = location->Offset;
            result.Size = location->Size;
            return result;
        } else {
            return Nothing();
        }
    }

    size_t DocCount() const {
        return ChunkResolver_->Size();
    }

private:
    TDocChunkMappingSearcher* ChunkResolver_;
    TVector<const TErasureLocationResolver*> Resolvers_;
    TVector<const TPartOptimizedErasureLocationResolver*> PartOptimizedResolvers_;
};

} // namespace NDoom
