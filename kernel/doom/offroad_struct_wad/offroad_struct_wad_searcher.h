#pragma once

#include "offroad_struct_wad_iterator.h"
#include "compression_type.h"

#include <kernel/doom/wad/mapper.h>
#include <kernel/doom/chunked_wad/single_chunked_wad.h>
#include <kernel/doom/chunked_wad/chunked_wad.h>

#include <util/memory/blob.h>

namespace NDoom {


template <EWadIndexType indexType, class Data, class Serializer, EStructType structType, ECompressionType compressionType>
class TOffroadStructWadSearcher {
    static constexpr bool IsFixedSizeStruct = (structType == FixedSizeStructType);

public:
    using TIterator = TOffroadStructWadIterator<Data, Serializer, structType, compressionType>;
    using THit = typename TIterator::THit;
    using TTable = typename TIterator::TReader::TTable;
    using TModel = typename TIterator::TReader::TModel;

    TOffroadStructWadSearcher() {}

    template <typename...Args>
    TOffroadStructWadSearcher(Args&&...args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const IChunkedWad* wad) {
        Reset(TArrayRef<const TTable*>(), wad);
    }

    void Reset(const TTable* table, const IWad* wad) {
        WrapWad_.Reset(new TSingleChunkedWad(wad));
        if (table)
            Reset(MakeArrayRef(&table, 1), WrapWad_.Get());
        else
            Reset(TArrayRef<const TTable*>(), WrapWad_.Get());
    }

    void Reset(const IWad* wad) {
        WrapWad_.Reset(new TSingleChunkedWad(wad));
        Reset(WrapWad_.Get());
    }

    void Reset(TArrayRef<const TTable*> tables, const IChunkedWad* wad) {
        Y_VERIFY(tables.empty() || tables.size() == wad->Chunks(), "Size of passed tables and number of wad chunks are not the same");
        Tables_.assign(tables.begin(), tables.end());

        Wad_ = wad;
        DocLumpMappingFromWad_.ConstructInPlace();
        Wad_->MapDocLumps({ TWadLumpId(indexType, EWadLumpRole::Struct) }, *DocLumpMappingFromWad_);

        if (Tables_.empty()) {
            Tables_.resize(Wad_->Chunks(), nullptr);
            LocalTables_.resize(Wad_->Chunks());
            TModel model;
            for (ui32 chunkId = 0; chunkId < Wad_->Chunks(); chunkId++) {
                if (compressionType != RawCompressionType) {
                    model.Load(Wad_->LoadChunkGlobalLump(chunkId, TWadLumpId(indexType, EWadLumpRole::StructModel)));
                }
                LocalTables_[chunkId].Reset(model);
                Tables_[chunkId] = &LocalTables_[chunkId];
            }
        }


        StructSizes_.resize(Wad_->Chunks(), Max<ui32>());

        if (IsFixedSizeStruct) {
            for (ui32 chunkId = 0; chunkId < Wad_->Chunks(); chunkId++) {
                TBlob structSizeRegion = Wad_->LoadChunkGlobalLump(chunkId, TWadLumpId(indexType, EWadLumpRole::StructSize));
                Y_ENSURE(structSizeRegion.Size() == sizeof(StructSizes_[chunkId]));
                StructSizes_[chunkId] = ReadUnaligned<ui32>(structSizeRegion.Data());
            }
        }
    }

    void MapDocLumps(const IDocLumpMapper& mapper) {
        DocLumpMappingFromMapper_.ConstructInPlace();
        mapper.MapDocLumps({ TWadLumpId(indexType, EWadLumpRole::Struct) }, *DocLumpMappingFromMapper_);
    }

    void Reset(const IChunkedWad* wad, const IDocLumpMapper* mapper) {
        Wad_ = wad;
        MapDocLumps(*mapper);

        if (Tables_.empty()) {
            Tables_.resize(Wad_->Chunks(), nullptr);
            LocalTables_.resize(Wad_->Chunks());
            TModel model;
            for (ui32 chunkId = 0; chunkId < Wad_->Chunks(); chunkId++) {
                if (compressionType != RawCompressionType) {
                    model.Load(Wad_->LoadChunkGlobalLump(chunkId, TWadLumpId(indexType, EWadLumpRole::StructModel)));
                }
                LocalTables_[chunkId].Reset(model);
                Tables_[chunkId] = &LocalTables_[chunkId];
            }
        }

        StructSizes_.resize(Wad_->Chunks(), Max<ui32>());

        if (IsFixedSizeStruct) {
            for (ui32 chunkId = 0; chunkId < Wad_->Chunks(); chunkId++) {
                TBlob structSizeRegion = Wad_->LoadChunkGlobalLump(chunkId, TWadLumpId(indexType, EWadLumpRole::StructSize));
                Y_ENSURE(structSizeRegion.Size() == sizeof(StructSizes_[chunkId]));
                StructSizes_[chunkId] = ReadUnaligned<ui32>(structSizeRegion.Data());
            }
        }
    }

    size_t Size() const {
        return Wad_->Size();
    }

    bool Find(ui32 docId, TIterator* iterator, bool useWadLumps = true) const {
        const auto chunkId = Wad_->DocChunk(docId);
        const auto* docDataHolder = iterator->DocDataHolder_;

        std::array<TArrayRef<const char>, 1> regions;
        if (DocLumpMappingFromWad_.Defined() && useWadLumps) {
            TBlob blob = Wad_->LoadDocLumps(docId, *DocLumpMappingFromWad_, regions);
            if (regions[0].empty()) {
                return false;
            }

            iterator->Blob_.Swap(blob);
        } else if (DocLumpMappingFromMapper_.Defined() && docDataHolder && docDataHolder->Contains(docId)) {
            const auto& loader = docDataHolder->Get(docId);

            loader.LoadDocRegions(*DocLumpMappingFromMapper_, regions);
            if (regions[0].empty()) {
                return false;
            }
        } else {
            return false;
        }
        iterator->Reader_.Reset(Tables_[chunkId], StructSizes_[chunkId], regions[0]);

        return true;
    }

private:
    TVector<TTable> LocalTables_;
    TVector<const TTable*> Tables_;

    const IChunkedWad* Wad_ = nullptr;
    THolder<IChunkedWad> WrapWad_;
    TVector<ui32> StructSizes_;
    TMaybe<std::array<size_t, 1>> DocLumpMappingFromWad_;
    TMaybe<std::array<size_t, 1>> DocLumpMappingFromMapper_;
};


} // namespace NDoom
