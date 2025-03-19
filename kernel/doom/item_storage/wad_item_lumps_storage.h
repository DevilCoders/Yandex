#pragma once

#include "backend.h"
#include "item_fetcher.h"
#include "lumps_set.h"
#include "item_storage.h"

namespace NDoom::NItemStorage {

class TEmptyItemLumpsAccessor : public IItemLumps {
public:
    TEmptyItemLumpsAccessor(TChunkId chunk)
        : Chunk_{ chunk }
    {}

    TChunkId ChunkId() const final {
        return Chunk_;
    }

    void LoadLumpsInternal(TConstArrayRef<size_t> /* lumps */, TArrayRef<TConstArrayRef<char>> lumps) const override {
        for (auto& region : lumps) {
            region = {};
        }
    }

private:
    TChunkId Chunk_;
};

class TItemLumpsAccessor : public TEmptyItemLumpsAccessor {
public:
    TItemLumpsAccessor(TChunkId chunk, TChunkedWadDocLumpLoader loader)
        : TEmptyItemLumpsAccessor{ chunk }
        , Loader_{ std::move(loader) }
    {}

    void LoadLumpsInternal(TConstArrayRef<size_t> mapping, TArrayRef<TConstArrayRef<char>> regions) const override {
        Loader_.LoadDocRegions(mapping, regions);
    }

private:
    TChunkedWadDocLumpLoader Loader_;
};

class TWadItemLumpsStorage : public IItemLumpsStorage {
public:
    TWadItemLumpsStorage(THolder<IItemFetcher> itemFetcher)
        : Fetcher_{std::move(itemFetcher)}
    {}

    void LoadItemLumps(NPrivate::TItemLumpsRequest& req, std::function<void(size_t, TStatusOr<IItemLumps*>)> callback) const override {
        Fetcher_->Load(req, [&](size_t i, TStatusOr<TItemLumps> res) {
            if (!res) {
                callback(i, res.Status());
                return;
            }

            if (!res->Item.Blob) {
                TEmptyItemLumpsAccessor accessor{res->Item.Chunk};
                callback(i, &accessor);
                return;
            }

            TChunkedWadDocLumpLoader loader{std::move(*res->Item.Blob), res->Item.Chunk.Id, res->Mapping.Remapping, res->Mapping.NumLumps};
            TItemLumpsAccessor accessor{res->Item.Chunk, std::move(loader)};
            callback(i, &accessor);
        });
    }

    void SetConsistentMapping(TConsistentItemLumpsMapping* consistentMapping) override {
        Fetcher_->SetConsistentMapping(consistentMapping);
    }

private:
    THolder<IItemFetcher> Fetcher_;
};


} // namespace NDoom::NItemStorage
