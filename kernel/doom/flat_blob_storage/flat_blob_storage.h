#pragma once

#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

namespace NDoom {

class IFlatBlobStorage {
public:
    virtual ui32 Chunks() const = 0;

    virtual void Read(
        TConstArrayRef<ui32> chunks,
        TConstArrayRef<ui32> parts,
        TConstArrayRef<ui64> offsets,
        TConstArrayRef<ui64> sizes,
        TArrayRef<TBlob> blobs) const = 0;

    void Read(
        TConstArrayRef<ui32> parts,
        TConstArrayRef<ui64> offsets,
        TConstArrayRef<ui64> sizes,
        TArrayRef<TBlob> blobs) const
    {
        return Read(TStackVec<ui32>(parts.size(), 0), parts, offsets, sizes, blobs);
    }

    virtual void Read(
        TConstArrayRef<ui32> chunks,
        TConstArrayRef<ui32> parts,
        TConstArrayRef<ui64> offsets,
        TConstArrayRef<ui64> sizes,
        std::function<void(size_t, TMaybe<TBlob>&&)> callback) const
    {
        TVector<TBlob> blobs(parts.size());
        Read(chunks, parts, offsets, sizes, blobs);
        for (size_t i = 0; i < parts.size(); ++i) {
            callback(i, std::move(blobs[i]));
        }
    }

    void Read(
        TConstArrayRef<ui32> parts,
        TConstArrayRef<ui64> offsets,
        TConstArrayRef<ui64> sizes,
        std::function<void(size_t, TMaybe<TBlob>&&)> callback) const
    {
        Read(TStackVec<ui32>(parts.size(), 0), parts, offsets, sizes, std::move(callback));
    }

    virtual ~IFlatBlobStorage() = default;
};

} // namespace NDoom
