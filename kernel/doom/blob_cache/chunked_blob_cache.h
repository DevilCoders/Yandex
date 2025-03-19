#pragma once

#include <util/memory/blob.h>
#include <util/generic/array_ref.h>
#include <array>

namespace NDoom {


class IChunkedBlobCache {
public:
    virtual ~IChunkedBlobCache() = default;

    bool TryLoadBlob(ui32 chunk, ui32 id, TBlob* blob) const {
        std::array<bool, 1> statuses;
        std::array<TBlob, 1> blobs;
        TryLoadBlobs({ chunk }, { id }, statuses, blobs);
        if (statuses[0]) {
            *blob = std::move(blobs[0]);
        }
        return statuses[0];
    }

    virtual void TryLoadBlobs(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids,
        TArrayRef<bool> statuses,
        TArrayRef<TBlob> blobs) const = 0;

    void StoreBlob(ui32 chunk, ui32 id, const TBlob& blob) {
        StoreBlobs({ chunk }, { id }, { blob }, { true });
    }

    virtual void StoreBlobs(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids,
        const TArrayRef<const TBlob>& blobs,
        TConstArrayRef<bool> statuses) = 0;
};


} // namespace NDoom
