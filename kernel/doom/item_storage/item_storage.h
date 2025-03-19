#pragma once

#include "backend.h"
#include "item_key.h"
#include "lumps_set.h"
#include "types.h"
#include "status.h"

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/yexception.h>

namespace NDoom::NItemStorage {

class IItemLumps {
public:
    virtual ~IItemLumps() = default;

    virtual TChunkId ChunkId() const = 0;

    virtual void LoadLumps(const TLumpsSet& set, TArrayRef<TConstArrayRef<char>> lumps) const;

    virtual void LoadLumpsInternal(TConstArrayRef<size_t> mapping, TArrayRef<TConstArrayRef<char>> lumps) const = 0;
};

class IRequester {
public:
    virtual ~IRequester() = default;

    virtual TLumpsSet MapItemLumps(TConstArrayRef<TLumpId> lumps) = 0;

    virtual void LoadItemLumps(TConstArrayRef<TItemId> itemIds, std::function<void(size_t, TStatusOr<IItemLumps*>)> callback) const = 0;
};

class IItemStorage {
public:
    virtual ~IItemStorage() = default;

    // Global lumps
    virtual TVector<TLumpId> GlobalLumps() const = 0;

    virtual bool HasGlobalLump(const TLumpId& lumpId) const = 0;

    virtual TBlob LoadGlobalLump(const TLumpId& lumpId) const = 0;

    // Chunk global lumps
    virtual TVector<TLumpId> ChunkGlobalLumps(TChunkId chunkId) const = 0;

    virtual bool HasChunkGlobalLump(TChunkId chunkId, const TLumpId& lumpId) const = 0;

    virtual TBlob LoadChunkGlobalLump(TChunkId chunkId, const TLumpId& lumpId) const = 0;

    // Item lumps
    virtual THolder<IRequester> MakeRequester() = 0;
};

////////////////////////////////////////////////////////////////////////////////

class TDuplicateItemTypeError : public yexception {};

class TItemStorageBuilder {
public:
    TItemStorageBuilder& AddBackend(TItemStorageBackend backend) {
        Backends_.push_back(std::move(backend));
        return *this;
    }

    THolder<IItemStorage> Build();

private:
    TVector<TItemStorageBackend> Backends_;
};

} // namespace NDoom::NItemStorage
