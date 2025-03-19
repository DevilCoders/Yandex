#pragma once

#include "blob_storage.h"
#include "consistent_lumps_mapping.h"
#include "request.h"
#include "status.h"

#include <kernel/doom/chunked_wad/mapper.h>

namespace NDoom::NItemStorage {

struct TItemLumpsMapping {
    TConstArrayRef<size_t> Remapping;
    size_t NumLumps = 0;
};

struct TItemLumps {
    TItemBlob Item;
    TItemLumpsMapping Mapping;
};

class IItemFetcher {
public:
    virtual ~IItemFetcher() = default;

    virtual void Load(NPrivate::TItemLumpsRequest& req, std::function<void(size_t item, TStatusOr<TItemLumps> lumps)> cb) const = 0;

    virtual void SetConsistentMapping(TConsistentItemLumpsMapping* mapping) = 0;
};

} // namespace NDoom::NItemStorage
