#include "item_storage.h"
#include "item_type_map.h"
#include "permutation.h"
#include "request.h"

#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <library/cpp/iterator/enumerate.h>

#include <util/generic/array_ref.h>
#include <util/generic/xrange.h>
#include <util/stream/output.h>
#include <util/generic/hash_set.h>

namespace NDoom::NItemStorage {

void IItemLumps::LoadLumps(const TLumpsSet& set, TArrayRef<TConstArrayRef<char>> lumps) const {
    return LoadLumpsInternal(set.GetMapping(), lumps);
}

namespace {

class TItemStorage final : public IItemStorage {
    class TRequester final : public IRequester {
    public:
        TRequester(TItemStorage* owner);

        TLumpsSet MapItemLumps(TConstArrayRef<TLumpId> lumps) override;

        void LoadItemLumps(TConstArrayRef<TItemId> itemIds, std::function<void(size_t, TStatusOr<IItemLumps*>)> callback) const override;

    private:
        TItemStorage* Owner_ = nullptr;
        TItemTypeMap<TVector<size_t>> LumpsIndices_;
        THashSet<size_t> MappedLumps_;
    };

public:
    TItemStorage(TVector<TItemStorageBackend> backends);
    void Reset(TVector<TItemStorageBackend> backends);

    // Global lumps
    TVector<TLumpId> GlobalLumps() const override;
    bool HasGlobalLump(const TLumpId& lumpId) const override;
    TBlob LoadGlobalLump(const TLumpId& lumpId) const override;

    // Chunk global lumps
    TVector<TLumpId> ChunkGlobalLumps(TChunkId chunkId) const override;
    bool HasChunkGlobalLump(TChunkId chunkId, const TLumpId& lumpId) const override;
    TBlob LoadChunkGlobalLump(TChunkId chunkId, const TLumpId& lumpId) const override;

    // Item lumps
    THolder<IRequester> MakeRequester() override;

protected:
    void LoadItemLumpsImpl(TConstArrayRef<TItemId> itemIds, const TItemTypeMap<TVector<size_t>>& lumps, std::function<void(size_t, TStatusOr<IItemLumps*>)> callback) const;
    void LoadItemLumpsSorted(TConstArrayRef<TItemId> itemIds, const TItemTypeMap<TVector<size_t>>& lumps, std::function<void(size_t, TStatusOr<IItemLumps*>)> callback) const;

    const TItemStorageBackend& Item(TItemType type) const;
    const TItemStorageBackend& Item(TLumpId lump) const;
    const TItemStorageBackend& Item(TItemId id) const;

private:
    TItemTypeMap<TItemStorageBackend> Backends_;
    TVector<TLumpId> GlobalLumps_;
    TConsistentItemLumpsMapping ConsistentMapping_;
};

TItemStorage::TRequester::TRequester(TItemStorage* storage)
    : Owner_{ storage }
{
}

TLumpsSet TItemStorage::TRequester::MapItemLumps(TConstArrayRef<TLumpId> lumps) {
    TVector<size_t> mapping(lumps.size(), 0);
    Owner_->ConsistentMapping_.RegisterItemLumps(lumps, mapping);

    for (const auto&& [i, lump] : Enumerate(lumps)) {
        if (MappedLumps_.contains(mapping[i])) {
            continue;
        }
        MappedLumps_.emplace(mapping[i]);
        LumpsIndices_[lump.ItemType].push_back(mapping[i]);
    }

    return TLumpsSet{ std::move(mapping) };
}


void TItemStorage::TRequester::LoadItemLumps(TConstArrayRef<TItemId> itemIds, std::function<void(size_t, TStatusOr<IItemLumps*>)> callback) const {
    return Owner_->LoadItemLumpsImpl(itemIds, LumpsIndices_, std::move(callback));
}

TItemStorage::TItemStorage(TVector<TItemStorageBackend> backends) {
    Reset(std::move(backends));
}

void TItemStorage::Reset(TVector<TItemStorageBackend> backends) {
    for (auto&& [i, backend] : Enumerate(backends)) {
        TItemType item = backend.ItemType();
        auto&& [ref, emplaced] = Backends_.emplace(item, std::move(backend));
        Y_ENSURE_EX(emplaced, TDuplicateItemTypeError{} << "Duplicate item type " << static_cast<ui32>(item));

        if (auto* globalLumps = ref.GlobalLumps()) {
            TVector<TLumpNameRef> lumps = globalLumps->GlobalLumps();
            for (TLumpNameRef lumpName : lumps) {
                GlobalLumps_.push_back(TLumpId{ item, TString{lumpName} });
            }
        }

        if (auto* items = ref.ItemLumps()) {
            items->SetConsistentMapping(&ConsistentMapping_);
        }
    }
}

TVector<TLumpId> TItemStorage::GlobalLumps() const {
    return GlobalLumps_;
}

bool TItemStorage::HasGlobalLump(const TLumpId& lumpId) const {
    const TItemStorageBackend* backend = Backends_.FindPtr(lumpId.ItemType);
    if (backend == nullptr) {
        return false;
    }

    return backend->GlobalLumps()->HasGlobalLump(lumpId.LumpName);
}

TBlob TItemStorage::LoadGlobalLump(const TLumpId& lumpId) const {
    return Item(lumpId).GlobalLumps()->LoadGlobalLump(lumpId.LumpName);
}

TVector<TLumpId> TItemStorage::ChunkGlobalLumps(TChunkId chunkId) const {
    IGlobalLumpsStorage* storage = Item(chunkId.ItemType).ChunkLump(chunkId.Id);
    Y_ENSURE(storage, "Unknown chunk " << chunkId.Id);

    TVector<TLumpNameRef> lumps = storage->GlobalLumps();
    TVector<TLumpId> result;
    result.reserve(lumps.size());
    for (TLumpNameRef name : lumps) {
        result.push_back(TLumpId{ chunkId.ItemType, TString{name} });
    }
    return result;
}

bool TItemStorage::HasChunkGlobalLump(TChunkId chunkId, const TLumpId& lumpId) const {
    Y_ENSURE(chunkId.ItemType == lumpId.ItemType, "Mismatched item types");

    IGlobalLumpsStorage* storage = Item(chunkId.ItemType).ChunkLump(chunkId.Id);
    Y_ENSURE(storage, "Unknown chunk " << chunkId.Id);

    return storage->HasGlobalLump(lumpId.LumpName);
}

TBlob TItemStorage::LoadChunkGlobalLump(TChunkId chunkId, const TLumpId& lumpId) const {
    Y_ENSURE(chunkId.ItemType == lumpId.ItemType, "Mismatched item types");

    IGlobalLumpsStorage* storage = Item(chunkId.ItemType).ChunkLump(chunkId.Id);
    Y_ENSURE(storage, "Unknown chunk " << chunkId.Id);

    return storage->LoadGlobalLump(lumpId.LumpName);
}

THolder<IRequester> TItemStorage::MakeRequester() {
    return MakeHolder<TRequester>( this );
}

void TItemStorage::LoadItemLumpsImpl(TConstArrayRef<TItemId> itemIds, const TItemTypeMap<TVector<size_t>>& lumps, std::function<void(size_t, TStatusOr<IItemLumps*>)> callback) const {
    TSmallVec<size_t> permutation(itemIds.size(), 0);
    Iota(permutation.begin(), permutation.end(), 0);

    auto compareByLoader = [this, &itemIds](size_t lhs) {
        return std::pair{Item(itemIds[lhs]).ItemLumps(), itemIds[lhs].ItemType};
    };

    if (Y_LIKELY(IsSortedBy(permutation.begin(), permutation.end(), compareByLoader))) {
        return LoadItemLumpsSorted(itemIds, lumps, std::move(callback));
    }

    SortBy(permutation.begin(), permutation.end(), compareByLoader);

    TSmallVec<TItemId> items = ApplyPermutation(itemIds, permutation);
    TSmallVec<size_t> inverse = ApplyPermutation(xrange(itemIds.size()), permutation);

    return LoadItemLumpsSorted(items, lumps, [&callback, &inverse](size_t index, TStatusOr<IItemLumps*> itemLumps) {
        callback(inverse[index], itemLumps);
    });
}

void TItemStorage::LoadItemLumpsSorted(TConstArrayRef<TItemId> itemIds, const TItemTypeMap<TVector<size_t>>& lumps, std::function<void(size_t, TStatusOr<IItemLumps*>)> callback) const {
    Y_ENSURE(itemIds);
    if (!itemIds) {
        return;
    }

    IItemLumpsStorage* loader = Item(itemIds[0]).ItemLumps();
    size_t left = 0;
    size_t right = 0;

    auto doLoad = [&] {
        if (left == right) {
            return;
        }

        NPrivate::TItemLumpsRequest req;
        TConstArrayRef<TItemId> items = itemIds.SubRegion(left, right - left);
        for (const TItemId& item : items) {
            req.AddItem(item);
        }
        for (TItemType type : req.ItemTypes()) {
            const TVector<size_t>* ptr = lumps.FindPtr(type);
            if (!ptr) {
                continue;
            }

            req.AddLumps(type, *ptr);
        }

        loader->LoadItemLumps(req, [&callback, left](size_t i, TStatusOr<IItemLumps*> lumps) {
            callback(i + left, lumps);
        });
    };
    for (; right < itemIds.size(); ++right) {
        if (auto nextLoader = Item(itemIds[right]).ItemLumps(); nextLoader != loader) {
            doLoad();
            left = right;
            loader = nextLoader;
        }
    }
    doLoad();
}

const TItemStorageBackend& TItemStorage::Item(TItemType type) const {
    const TItemStorageBackend* backend = Backends_.FindPtr(type);
    Y_ENSURE(backend, "Unknown item type " << static_cast<ui32>(type));
    return *backend;
}

const TItemStorageBackend& TItemStorage::Item(TLumpId id) const {
    return Item(id.ItemType);
}

const TItemStorageBackend& TItemStorage::Item(TItemId id) const {
    return Item(id.ItemType);
}

} // namespace 

THolder<IItemStorage> TItemStorageBuilder::Build() {
    return MakeHolder<TItemStorage>(std::move(Backends_));
}

} // namespace NDoom
