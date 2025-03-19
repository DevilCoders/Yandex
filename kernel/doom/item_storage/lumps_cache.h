#pragma once

#include "backend.h"
#include "item_storage.h"
#include "types.h"

#include <search/base/blob_cache/fragmented_blob_storage.h>

#include <library/cpp/containers/absl_flat_hash/flat_hash_set.h>
#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/iterator/zip.h>

#include <util/datetime/base.h>
#include <util/generic/deque.h>
#include <util/generic/xrange.h>
#include <util/system/mutex.h>

namespace NDoom::NItemStorage {

struct TLumpCacheOptions {
    TLumpName Lump;

    ui32 NumPages = 0;
    ui32 PageSize = 4096;
    ui32 MaxPagesPerLump = 0;

    TDuration ExpirationTime = TDuration::Seconds(60);
};

inline bool ShouldCache(EStatusCode statusCode) {
    return statusCode == EStatusCode::Ok
        || statusCode == EStatusCode::NotFound;
}

class TLumpCache {
private:
    static inline ui32 InvalidPage = Max<ui32>();

    struct TCacheItem {
        ui32 FirstPage = InvalidPage;
        TInstant ExpiresAt;
    };

public:
    TLumpCache(const TLumpCacheOptions& options, size_t numSlots)
        : MaxLumpSize_{static_cast<size_t>(options.PageSize) * options.MaxPagesPerLump}
        , ExpirationTime_{options.ExpirationTime}
        , Slots_(numSlots)
        , Storage_{options.PageSize, options.NumPages}
    {
        Y_ENSURE(numSlots > 0);
    }

    bool AllowedToStore(size_t size) const {
        return size < MaxLumpSize_;
    }

    bool CanStore(size_t size) {
        return Storage_.CanWrite(size);
    }

    TMaybe<TBlob> Load(size_t pos, TInstant now) {
        Y_ASSERT(pos < Slots_.size());

        auto& slot = Slots_[pos];
        if (slot.FirstPage == InvalidPage) {
            return Nothing();
        }

        if (slot.ExpiresAt <= now) {
            Remove(pos);
            return Nothing();
        }
        return Storage_.ReadBlob(slot.FirstPage);
    }

    void Store(size_t pos, TBlob blob, TInstant now) {
        Y_ASSERT(pos < Slots_.size());
        Remove(pos);

        auto& slot = Slots_[pos];
        slot.FirstPage = Storage_.WriteBlob(std::move(blob));
        slot.ExpiresAt = now + ExpirationTime_;
    }

    void Remove(size_t pos) {
        Y_ASSERT(pos < Slots_.size());
        auto& slot = Slots_[pos];
        if (slot.FirstPage != InvalidPage) {
            Storage_.DeleteBlob(slot.FirstPage);
            slot.FirstPage = InvalidPage;
        }
    }

private:
    const size_t MaxLumpSize_ = 0;
    const TDuration ExpirationTime_;

    TVector<TCacheItem> Slots_;
    NBaseSearch::TFragmentedBlobStorage Storage_;
};

class TCachedItemLumps : public IItemLumps {
public:
    TCachedItemLumps(TChunkId chunkId, TStackVec<TBlob, 4> lumps = {}, bool isCached = false)
        : Chunk_{chunkId}
        , Lumps_{std::move(lumps)}
        , IsCached_{isCached}
    {
    }

    TChunkId ChunkId() const override {
        return Chunk_;
    }

    bool IsCached() const {
        return IsCached_;
    }

    void LoadLumpsInternal(const TConstArrayRef<size_t> mapping, TArrayRef<TConstArrayRef<char>> regions) const override {
        for (auto [i, region] : Zip(mapping, regions)) {
            if (i >= Lumps_.size()) {
                region = {};
            } else {
                region = TConstArrayRef<char>{Lumps_[i].AsCharPtr(), Lumps_[i].Size()};
            }
        }
    }

    TConstArrayRef<TBlob> Lumps() const {
        return Lumps_;
    }

    void SetLump(size_t index, TBlob blob) {
        if (Lumps_.size() <= index) {
            Lumps_.resize(index + 1);
        }
        Lumps_[index] = std::move(blob);
    }

private:
    TChunkId Chunk_;
    TStackVec<TBlob, 4> Lumps_;
    bool IsCached_ = false;
};

// Handles caching inside one item type
class TTableCache {
    inline static constexpr size_t InvalidSlot = Max<size_t>();

    struct TCachedValue {
        TChunkId Chunk;
    };

    struct TCacheNode : public TIntrusiveListItem<TCacheNode> {
        TItemKey Key = 0;
        size_t Slot = InvalidSlot;
        TStatusOr<TChunkId> Chunk = TStatus::Err(EStatusCode::Unknown);
    };

public:
    TTableCache(TItemType type, size_t numSlots, TVector<TLumpCacheOptions> lumpCaches)
        : ItemType_{type}
    {
        for (TLumpCacheOptions& options : lumpCaches) {
            LumpNames_.push_back(options.Lump);
            Caches_.emplace_back(options, numSlots);
        }
        for (size_t i : xrange(numSlots)) {
            TCacheNode& node = Nodes_.emplace_back();
            node.Chunk = TStatus::Err(EStatusCode::Unknown);
            node.Slot = i;
            FreeNodes_.PushFront(&node);
        }
    }

    void SetConsistentMapping(TConsistentItemLumpsMapping* mapping) {
        LumpsIds_.resize(LumpNames_.size());

        TVector<TLumpId> lumps;
        for (TLumpName name : LumpNames_) {
            lumps.push_back(TLumpId{
                .ItemType = ItemType_,
                .LumpName = std::move(name),
            });
        }

        mapping->RegisterItemLumps(lumps, LumpsIds_);

        for (auto [i, j] : Enumerate(LumpsIds_)) {
            if (LumpIdToCacheIndex_.size() <= j) {
                LumpIdToCacheIndex_.resize(j + 1, Max<ui32>());
            }
            LumpIdToCacheIndex_[j] = i;
        }
    }

    void Store(TConstArrayRef<TItemKey> keys, TConstArrayRef<size_t> lumps, TArrayRef<TStatusOr<TCachedItemLumps>> results, TInstant now) {
        TGuard guard{Lock_};

        for (auto [key, result] : Zip(keys, results)) {
            StoreItem(key, lumps, result, now);
        }
    }

    void Load(TConstArrayRef<TItemKey> keys, TConstArrayRef<size_t> lumps, TArrayRef<TStatusOr<TCachedItemLumps>> results, TInstant now) {
        if (!AllLumpsArePresent(lumps)) {
            return;
        }

        TGuard guard{Lock_};

        for (auto [key, result] : Zip(keys, results)) {
            TryLoadItem(key, lumps, result, now);
        }
    }

    TConstArrayRef<size_t> LumpsIndices() const {
        return LumpsIds_;
    }

    TItemType ItemType() const {
        return ItemType_;
    }

private:
    bool AllLumpsArePresent(TConstArrayRef<size_t> lumps) const {
        return AllOf(lumps, [this](size_t lump) {
            return IsKnownLump(lump);
        });
    }

    bool IsKnownLump(size_t lump) const {
        return lump < LumpIdToCacheIndex_.size()
            && LumpIdToCacheIndex_[lump] != Max<ui32>();
    }

    void TryLoadItem(TItemKey key, TConstArrayRef<size_t> lumps, TStatusOr<TCachedItemLumps>& result, TInstant now) {
        TCacheNode* node = nullptr;
        if (auto it = Mapping_.find(key); it != Mapping_.end()) {
            node = *it;
        } else {
            return;
        }

        if (node->Chunk.IsErr()) {
            result = node->Chunk.Status();
            return;
        }

        TSmallVec<TBlob> blobs;
        for (size_t lump : lumps) {
            const size_t cacheIndex = LumpIdToCacheIndex_.at(lump);
            TLumpCache& cache = Caches_.at(cacheIndex);

            TMaybe<TBlob> blob = cache.Load(node->Slot, now);
            if (!blob) {
                return;
            }

            if (blobs.size() <= lump) {
                blobs.resize(lump + 1);
            }
            blobs[lump] = std::move(*blob);
        }

        result.EmplaceOk(node->Chunk.Unwrap(), std::move(blobs), /*isCached=*/ true);
    }

    void StoreItem(TItemKey key, TConstArrayRef<size_t> lumps, TStatusOr<TCachedItemLumps>& result, TInstant now) {
        if (!ShouldCache(result.StatusCode())) {
            return;
        }

        TCacheNode* node = nullptr;
        if (auto it = Mapping_.find(key); it != Mapping_.end()) {
            node = *it;
        } else {
            node = AcquireNode(key);
        }
        UsedNodes_.PushFront(node);

        if (result.IsErr()) {
            if (node->Chunk.IsOk()) {
                for (auto& cache : Caches_) {
                    cache.Remove(node->Slot);
                }
            }
            node->Chunk.EmplaceErr(std::move(result).Status());
            return;
        }
        node->Chunk.EmplaceOk(result->ChunkId());
        if (result->IsCached()) {
            return;
        }

        TConstArrayRef<TBlob> blobs = result->Lumps();
        for (size_t lump : lumps) {
            if (!IsKnownLump(lump)) {
                continue;
            }

            const TBlob& blob = blobs[lump];
            if (blob.Empty()) {
                continue;
            }

            const size_t cacheIndex = LumpIdToCacheIndex_.at(lump);
            TLumpCache& cache = Caches_.at(cacheIndex);

            if (!cache.AllowedToStore(blob.size())) {
                continue;
            }

            while (!cache.CanStore(blob.size())) {
                FreeLastNode();
            }

            cache.Store(node->Slot, std::move(blob), now);
        }
    }

    TCacheNode* AcquireNode(TItemKey key) {
        if (FreeNodes_.Empty()) {
            FreeLastNode();
        }
        Y_ENSURE(!FreeNodes_.Empty());
        TCacheNode* node = FreeNodes_.PopBack();

        node->Key = key;
        Mapping_.insert(node);

        return node;
    }

    void FreeLastNode() {
        Y_VERIFY(!UsedNodes_.Empty());
        TCacheNode* node = UsedNodes_.PopBack();
        Y_ENSURE(Mapping_.erase(node) == 1);

        for (auto& cache : Caches_) {
            cache.Remove(node->Slot);
        }

        FreeNodes_.PushFront(node);
    }

private:
    struct TCacheNodePtrHasher {
        using is_transparent = void;

        size_t operator()(const TItemKey& key) const {
            auto hash = []<typename T>(const T& value) {
                return THash<T>{}(value);
            };

            return hash(key);
        }

        size_t operator()(const TCacheNode* entry) const {
            return (*this)(entry->Key);
        }
    };

    struct TCacheNodePtrEq {
        using is_transparent = void;

        bool operator()(const TCacheNode* lhs, const TCacheNode* rhs) const {
            return lhs->Key == rhs->Key;
        }

        bool operator()(const TCacheNode* lhs, const TItemKey& rhs) const {
            return (*this)(rhs, lhs);
        }

        bool operator()(const TItemKey& lhs, const TCacheNode* rhs) const {
            return lhs == rhs->Key;
        }
    };

private:
    const TItemType ItemType_ = TItemType{0};
    TVector<TString> LumpNames_;
    TVector<size_t> LumpsIds_;

    TVector<ui32> LumpIdToCacheIndex_;
    TVector<TLumpCache> Caches_;

    TMutex Lock_;
    TDeque<TCacheNode> Nodes_;
    TIntrusiveList<TCacheNode> FreeNodes_;
    TIntrusiveList<TCacheNode> UsedNodes_;
    absl::flat_hash_set<TCacheNode*, TCacheNodePtrHasher, TCacheNodePtrEq> Mapping_;
};

class TTableCacheBuilder {
public:
    TTableCacheBuilder& SetItemType(TItemType type) {
        ItemType_ = type;
        return *this;
    }

    TTableCacheBuilder& SetNumSlots(size_t numSlots) {
        NumSlots_ = numSlots;
        return *this;
    }

    TTableCacheBuilder& AddLump(TLumpCacheOptions options) {
        LumpCaches_.push_back(std::move(options));
        return *this;
    }

    THolder<TTableCache> Build() {
        return MakeHolder<TTableCache>(ItemType_, NumSlots_, std::move(LumpCaches_));
    }

private:
    TItemType ItemType_ = 0;
    size_t NumSlots_ = 0;
    TVector<TLumpCacheOptions> LumpCaches_;
};


class TCachedLumpsStorage : public IItemLumpsStorage {
public:
    TCachedLumpsStorage(IItemLumpsStoragePtr slave, TArrayRef<THolder<TTableCache>> caches)
        : Slave_{std::move(slave)}
    {
        for (auto&& cache : caches) {
            Caches_.emplace(cache->ItemType(), std::move(cache));
        }
    }

    void LoadItemLumps(NPrivate::TItemLumpsRequest& req, std::function<void(size_t, TStatusOr<IItemLumps*>)> callback) const override {
        const auto now = Now();

        // FIXME(sskvor): Use common context
        NPrivate::TItemLumpsRequest slaveRequest;

        TSmallVec<size_t> slaveRequestIndices;
        slaveRequestIndices.reserve(req.NumItems());

        size_t i = 0;
        TItemTypeMap<size_t> offsets;
        TSmallVec<TStatusOr<TCachedItemLumps>> results(req.NumItems(), TStatus::Err(EStatusCode::Unknown));
        TSmallVec<TItemType> types(req.NumItems(), 0);

        for (TItemType type : req.ItemTypes()) {
            TConstArrayRef<TItemKey> keys = req.ItemTypeRequest(type).Items;
            TConstArrayRef<size_t> lumps = req.ItemTypeRequest(type).Lumps;

            slaveRequest.AddLumps(type, lumps);

            std::fill(types.begin() + i, types.begin() + i + keys.size(), type);

            offsets[type] = i;
            auto cachedLumps = TArrayRef<TStatusOr<TCachedItemLumps>>{results}.subspan(i, keys.size());

            auto* cache = Caches_.FindPtr(type);
            if (!cache) {
                for (TItemKey key : keys) {
                    slaveRequest.AddItem(type, key);
                    slaveRequestIndices.push_back(i++);
                }
                continue;
            }

            (*cache)->Load(keys, lumps, cachedLumps, now);

            for (auto&& [key, res] : Zip(keys, cachedLumps)) {
                if (ShouldCache(res.StatusCode())) {
                    TStatusOr<IItemLumps*> ptr = res.Map([](TCachedItemLumps& lumps) -> IItemLumps* {
                        return &lumps;
                    });
                    callback(i, ptr);
                } else {
                    slaveRequest.AddItem(type, key);
                    slaveRequestIndices.push_back(i);
                }
                ++i;
            }
        }

        Slave_->LoadItemLumps(slaveRequest, [&](size_t i, TStatusOr<IItemLumps*> lumps) {
            i = slaveRequestIndices[i];
            Y_ENSURE(results[i].StatusCode() == EStatusCode::Unknown, "Unexpected code " << results[i].Status());

            callback(i, lumps);

            if (lumps.IsErr()) {
                results[i] = lumps.Status();
            } else if (auto* cache = Caches_.FindPtr(types[i])) {
                TConstArrayRef<size_t> mapping = (*cache)->LumpsIndices();
                TSmallVec<TConstArrayRef<char>> regions(mapping.size());

                auto* loader = lumps.Unwrap();
                loader->LoadLumpsInternal(mapping, regions);

                results[i].EmplaceOk(loader->ChunkId());
                for (auto [lumpIndex, lumpBlob] : Zip(mapping, regions)) {
                    results[i]->SetLump(lumpIndex, TBlob::Copy(lumpBlob.data(), lumpBlob.size()));
                }
            }
        });

        for (TItemType type : req.ItemTypes()) {
            TConstArrayRef<TItemKey> keys = req.ItemTypeRequest(type).Items;
            TConstArrayRef<size_t> lumps = req.ItemTypeRequest(type).Lumps;

            auto cachedLumps = TArrayRef<TStatusOr<TCachedItemLumps>>{results}.subspan(offsets[type], keys.size());

            auto* cache = Caches_.FindPtr(type);
            if (!cache) {
                i += keys.size();
                continue;
            }

            (*cache)->Store(keys, lumps, cachedLumps, now);
        }
    }

    void SetConsistentMapping(TConsistentItemLumpsMapping* consistentMapping) override {
        for (TItemType type : Caches_.ItemTypes()) {
            Caches_.at(type)->SetConsistentMapping(consistentMapping);
        }
        Slave_->SetConsistentMapping(consistentMapping);
    }

private:
    IItemLumpsStoragePtr Slave_;
    mutable TItemTypeMap<THolder<TTableCache>> Caches_;
};

}
