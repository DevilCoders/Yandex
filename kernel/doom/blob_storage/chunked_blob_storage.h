#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>


namespace NDoom {


class IChunkedBlobStorage {
public:
    virtual ~IChunkedBlobStorage() = default;

    virtual ui32 Chunks() const = 0;

    virtual ui32 ChunkSize(ui32 chunk) const = 0;

    virtual TBlob Read(ui32 chunk, ui32 id) const = 0;

    virtual TVector<TBlob> Read(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids) const
    {
        TVector<TBlob> result;
        for (size_t i = 0; i < chunks.size(); ++i) {
            result.push_back(Read(chunks[i], ids[i]));
        }
        return result;
    }

    virtual void Read(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids,
        std::function<void(size_t, TMaybe<TBlob>&&)> callback) const
    {
        TVector<TBlob> result = Read(chunks, ids);
        for (size_t i = 0; i < chunks.size(); ++i) {
            callback(i, std::move(result[i]));
        }
    }
};


} // namespace NDoom
