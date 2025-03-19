#pragma once

#include "request.h"
#include "status.h"
#include "types.h"

#include <util/generic/maybe.h>
#include <util/memory/blob.h>

#include <functional>

namespace NDoom::NItemStorage {

struct TItemBlob {
    TChunkId Chunk;
    TMaybe<TBlob> Blob; // Empty iif no lumps requested
};

class IBlobStorage {
public:
    virtual ~IBlobStorage() = default;

    virtual void Load(NPrivate::TItemLumpsRequest& req, std::function<void(size_t item, TStatusOr<TItemBlob> blob)> cb) = 0;
};

} // namespace NDoom::NItemStorage
