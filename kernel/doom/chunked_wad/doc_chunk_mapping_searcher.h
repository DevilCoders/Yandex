#pragma once

#include "doc_chunk_mapping.h"

#include <kernel/doom/wad/wad.h>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/flat/flat_searcher.h>

#include <kernel/doom/chunked_wad/protos/doc_chunk_mapping_union.pb.h>

namespace NDoom {


class TDocChunkMappingSearcher {
    using TSearcher = NOffroad::TFlatSearcher<TDocChunkMapping, std::nullptr_t, TDocChunkMappingVectorizer, NOffroad::TNullVectorizer>;
public:

    TDocChunkMappingSearcher() = default;

    TDocChunkMappingSearcher(const TString& path, bool lockMemory = false) {
        Reset(path, lockMemory);
    }

    TDocChunkMappingSearcher(const IWad* wad) {
        Reset(wad);
    }

    void Reset(const TString& path, bool lockMemory = false) {
        LocalWad_ = IWad::Open(path, lockMemory);
        ResetInternal(LocalWad_.Get());
    }

    void Reset(const IWad* wad) {
        LocalWad_.Reset();
        ResetInternal(wad);
    }

    size_t Size() const {
        if (ProtoMapping_.GetMappingCase() == TDocChunkMappingUnion::kModuloMapping) {
            return ProtoMapping_.GetModuloMapping().GetSize();
        } else if (ProtoMapping_.GetMappingCase() == TDocChunkMappingUnion::kSequentialMapping) {
            return ProtoMapping_.GetSequentialMapping().GetIndices()[ProtoMapping_.GetSequentialMapping().GetIndices().size()-1];
        } else {
            return Searcher_.Size();
        }
    }

    TDocChunkMapping Find(size_t docId) const {
        if (ProtoMapping_.GetMappingCase() == TDocChunkMappingUnion::kModuloMapping) {
            const TModuloDocChunkMappingInfo& mapping = ProtoMapping_.GetModuloMapping();
            return TDocChunkMapping(docId % mapping.GetModulo(), docId / mapping.GetModulo());
        } else if (ProtoMapping_.GetMappingCase() == TDocChunkMappingUnion::kSequentialMapping) {
            const TSequentialDocChunkMappingInfo& mapping = ProtoMapping_.GetSequentialMapping();
            auto iterator = UpperBound(mapping.GetIndices().begin(), mapping.GetIndices().end(), docId);
            if (iterator == mapping.GetIndices().end() ||
                iterator == mapping.GetIndices().begin()) {
                return TDocChunkMapping::Invalid;
            }
            --iterator;
            const ui32 distance = iterator - mapping.GetIndices().begin();
            return TDocChunkMapping(distance, docId - *iterator);
        } else {
            return Searcher_.ReadKey(docId);
        }
    }

private:
    void ResetInternal(const IWad* wad) {
        if (wad->HasGlobalLump(TWadLumpId(ChunkMappingIndexType, EWadLumpRole::Struct))) {
            const TBlob structBlob = wad->LoadGlobalLump(TWadLumpId(ChunkMappingIndexType, EWadLumpRole::Struct));

            Y_ENSURE(ProtoMapping_.ParseFromArray(structBlob.Data(), structBlob.Size()));

            // for backwards compatibility
            if (ProtoMapping_.GetMappingCase() == TDocChunkMappingUnion::MAPPING_NOT_SET) {
                TModuloDocChunkMappingInfo moduloMapping;
                Y_ENSURE(moduloMapping.ParseFromArray(structBlob.Data(), structBlob.Size()));
                *ProtoMapping_.MutableModuloMapping() = moduloMapping;
            }

            switch (ProtoMapping_.GetMappingCase()) {
                case TDocChunkMappingUnion::kModuloMapping: {
                    const TModuloDocChunkMappingInfo& moduloMapping = ProtoMapping_.GetModuloMapping();
                    Y_ENSURE(moduloMapping.HasModulo());
                    Y_ENSURE(moduloMapping.HasSize());
                    Y_ENSURE(moduloMapping.GetModulo() > 0);
                    break;
                }
                case TDocChunkMappingUnion::kSequentialMapping: {
                    const TSequentialDocChunkMappingInfo& sequentialMapping = ProtoMapping_.GetSequentialMapping();
                    Y_ENSURE(!sequentialMapping.GetIndices().empty());
                    Y_ENSURE(IsSorted(sequentialMapping.GetIndices().begin(), sequentialMapping.GetIndices().end()));
                    break;
                }
                default:
                    Y_ENSURE(false);
            }

        } else {
            Y_ENSURE(wad->HasGlobalLump(TWadLumpId(ChunkMappingIndexType, EWadLumpRole::Hits)));
            Searcher_.Reset(wad->LoadGlobalLump(TWadLumpId(ChunkMappingIndexType, EWadLumpRole::Hits)));
        }
    }

    THolder<IWad> LocalWad_;
    TSearcher Searcher_;
    TDocChunkMappingUnion ProtoMapping_;
};


} // namespace NDoom
