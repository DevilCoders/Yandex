#pragma once

#include "consistent_lumps_mapping.h"
#include "request.h"
#include "status.h"
#include "types.h"

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

namespace NDoom::NItemStorage {

class IItemLumps;

class IItemLumpsStorage : public TSimpleRefCount<IItemLumpsStorage> {
public:
    virtual ~IItemLumpsStorage() = default;

    virtual void LoadItemLumps(NPrivate::TItemLumpsRequest& req, std::function<void(size_t, TStatusOr<IItemLumps*>)> callback) const = 0;

    virtual void SetConsistentMapping(TConsistentItemLumpsMapping* consistentMapping) = 0;
};

using IItemLumpsStoragePtr = TIntrusivePtr<IItemLumpsStorage>;

////////////////////////////////////////////////////////////////////////////////

class IGlobalLumpsStorage : public TSimpleRefCount<IGlobalLumpsStorage> {
public:
    virtual ~IGlobalLumpsStorage() = default;

    virtual TVector<TLumpNameRef> GlobalLumps() const = 0;
    virtual bool HasGlobalLump(TLumpNameRef lumpName) const = 0;
    virtual TBlob LoadGlobalLump(TLumpNameRef lumpName) const = 0;
};

using IGlobalLumpsStoragePtr = TIntrusivePtr<IGlobalLumpsStorage>;

////////////////////////////////////////////////////////////////////////////////

class TItemStorageBackend {
public:
    TItemStorageBackend(TItemType itemType, IGlobalLumpsStoragePtr globalLumps, TVector<IGlobalLumpsStoragePtr> chunkLumps, IItemLumpsStoragePtr itemLumps)
        : ItemType_{ itemType }
        , GlobalLumps_{ std::move(globalLumps) }
        , ChunkGlobalLumps_{ std::move(chunkLumps) }
        , ItemLumps_{ std::move(itemLumps) }
    {}

    TItemType ItemType() const {
        return ItemType_;
    }

    IItemLumpsStorage* ItemLumps() const {
        return ItemLumps_.Get();
    }

    IGlobalLumpsStorage* GlobalLumps() const {
        return GlobalLumps_.Get();
    }

    IGlobalLumpsStorage* ChunkLump(ui32 chunk) const {
        return chunk < ChunkGlobalLumps_.size() ? ChunkGlobalLumps_[chunk].Get() : nullptr;
    }

    TVector<IGlobalLumpsStoragePtr> ChunkGlobalLumps() const {
        return ChunkGlobalLumps_;
    }

private:
    TItemType ItemType_ = 0;
    IGlobalLumpsStoragePtr GlobalLumps_;
    TVector<IGlobalLumpsStoragePtr> ChunkGlobalLumps_;
    IItemLumpsStoragePtr ItemLumps_;
};

////////////////////////////////////////////////////////////////////////////////

class TItemStorageBackendBuilder {
public:
    TItemStorageBackendBuilder& SetGlobalLumpsStorage(IGlobalLumpsStoragePtr storage) {
        GlobalLumps_ = std::move(storage);
        return *this;
    }

    TItemStorageBackendBuilder& SetChunkGlobalLumps(TVector<IGlobalLumpsStoragePtr> chunks) {
        ChunkGlobalLumps_ = std::move(chunks);
        return *this;
    }

    TItemStorageBackendBuilder& SetItemLumps(IItemLumpsStoragePtr storage) {
        ItemLumps_ = std::move(storage);
        return *this;
    }

    TItemStorageBackendBuilder& SetItemType(TItemType type) {
        ItemType_ = type;
        return *this;
    }

    TItemStorageBackend Build() {
        Y_ENSURE(ItemType_, "No item type set");

        return TItemStorageBackend{
            ItemType_.GetRef(),
            std::move(GlobalLumps_),
            std::move(ChunkGlobalLumps_),
            std::move(ItemLumps_),
        };
    }

private:
    TMaybe<TItemType> ItemType_ = 0;
    IGlobalLumpsStoragePtr GlobalLumps_;
    TVector<IGlobalLumpsStoragePtr> ChunkGlobalLumps_;
    IItemLumpsStoragePtr ItemLumps_;
};

} // namespace NDoom::NItemStorage
