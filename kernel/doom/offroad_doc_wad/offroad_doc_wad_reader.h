#pragma once

#include <array>

#include <kernel/doom/chunked_wad/chunked_wad.h>
#include <kernel/doom/chunked_wad/single_chunked_wad.h>
#include <kernel/doom/wad/mega_wad_reader.h>
#include "offroad_doc_wad_mapping.h"
#include "offroad_doc_codec.h"
#include "offroad_doc_common.h"

namespace NDoom {

template<EWadIndexType indexType, class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer, EOffroadDocCodec codec = AdaptiveDocCodec>
class TOffroadDocWadReader {
    using TReader = typename TOffroadDocCommon<codec, Hit, Vectorizer, Subtractor, PrefixVectorizer>::TReader;
    using TMapping = TOffroadDocWadMapping<indexType, PrefixVectorizer>;
public:
    using THit = Hit;
    using TTable = typename TReader::TTable;
    using TModel = typename TReader::TModel;

    enum {
        HasLowerBound = true
    };

    TOffroadDocWadReader() {}

    template <typename...Args>
    TOffroadDocWadReader(Args&&...args) {
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
        WadReader_.Reset(wad, Mapping_.DocLumps());

        if (Tables_.empty()) {
            Tables_.resize(Wad_->Chunks(), nullptr);
            LocalTables_.resize(Wad_->Chunks());
            TModel model;
            for (ui32 chunkId = 0; chunkId < Wad_->Chunks(); chunkId++) {
                model.Load(Wad_->LoadChunkGlobalLump(chunkId, Mapping_.ModelLump()));
                LocalTables_[chunkId].Reset(model);
                Tables_[chunkId] = &LocalTables_[chunkId];
            }
        }
    }

    void Restart() {
        WadReader_.Restart();
    }

    bool ReadDoc(ui32* docId) {
        if (WadReader_.ReadDoc(docId, &Regions_)) {
            const size_t chunkId = Wad_->DocChunk(*docId);
            Mapping_.ResetStream(&Reader_, Tables_[chunkId], Regions_);
            return true;
        } else {
            return false;
        }
    }

    bool ReadHit(THit* hit) {
        return Reader_.ReadHit(hit);
    }

    template <class Consumer>
    bool ReadHits(const Consumer& consumer) {
        return Reader_.ReadHits(consumer);
    }

    TProgress Progress() const {
        return WadReader_.Progress();
    }

    TBlob LoadGlobalLump(TWadLumpId id) const {
        return WadReader_.LoadGlobalLump(id);
    }

private:
    THolder<IChunkedWad> LocalWad_;
    const IChunkedWad* Wad_ = nullptr;
    THolder<IChunkedWad> WrapWad_;

    TVector<TTable> LocalTables_;
    TVector<const TTable*> Tables_;
    TMapping Mapping_;

    TMegaWadReader WadReader_;
    std::array<TArrayRef<const char>, TMapping::DocLumpCount> Regions_;
    TReader Reader_;
};


} // namespace NDoom
