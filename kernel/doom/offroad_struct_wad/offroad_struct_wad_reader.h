#pragma once

#include "struct_reader.h"

#include <kernel/doom/chunked_wad/chunked_wad.h>
#include <kernel/doom/chunked_wad/single_chunked_wad.h>
#include <kernel/doom/wad/mega_wad_reader.h>


namespace NDoom {

template <EWadIndexType indexType, class Data, class Serializer, EStructType structType, ECompressionType compressionType>
class TOffroadStructWadReader {
    using TReader = TStructReader<Data, Serializer, structType, compressionType>;
    static constexpr bool IsFixedSizeStruct = (structType == FixedSizeStructType);

public:
    using THit = Data;
    using TTable = typename TReader::TTable;
    using TModel = typename TReader::TModel;

    enum {
        HasLowerBound = false
    };

    TOffroadStructWadReader() {}

    template <typename...Args>
    TOffroadStructWadReader(Args&&...args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const TString& path) {
        LocalWad_ = IChunkedWad::Open(path);
        Reset(LocalWad_.Get());
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

    void Reset(const IChunkedWad* wad) {
        Reset(TArrayRef<const TTable*>(), wad);
    }

    void Reset(TArrayRef<const TTable*> tables, const IChunkedWad* wad) {
        Y_VERIFY(tables.empty() || tables.size() == wad->Chunks(), "Size of passed tables and number of wad chunks are not the same");
        Tables_.assign(tables.begin(), tables.end());

        Wad_ = wad;

        TVector<TWadLumpId> docLumps = { TWadLumpId(indexType, EWadLumpRole::Struct) };
        WadReader_.Reset(Wad_, docLumps);

        if (Tables_.empty()) {
            Tables_.resize(Wad_->Chunks(), nullptr);
            LocalTables_.resize(Wad_->Chunks());
            TModel model;
            for (ui32 chunkId = 0; chunkId < Wad_->Chunks(); chunkId++) {
                model.Load(Wad_->LoadChunkGlobalLump(chunkId, TWadLumpId(indexType, EWadLumpRole::StructModel)));
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

    void Restart() {
        WadReader_.Restart();
    }

    bool ReadDoc(ui32* docId) {
        std::array<TArrayRef<const char>, 1> regions;
        if (WadReader_.ReadDoc(docId, &regions)) {
            const size_t chunkId = Wad_->DocChunk(*docId);
            Reader_.Reset(Tables_[chunkId], StructSizes_[chunkId], regions[0]);
            return true;
        } else {
            return false;
        }
    }

    bool ReadHit(THit* hit) {
        return Reader_.Read(hit);
    }

    TProgress Progress() const {
        return WadReader_.Progress();
    }

    size_t Size() const {
        return Wad_->Size();
    }

private:
    THolder<IChunkedWad> LocalWad_;
    const IChunkedWad* Wad_ = nullptr;
    THolder<IChunkedWad> WrapWad_;

    TVector<TTable> LocalTables_;
    TVector<const TTable*> Tables_;

    TMegaWadReader WadReader_;
    TVector<ui32> StructSizes_;
    TReader Reader_;
};


} // namespace NDoom
