#pragma once

#include "chunked_blob_storage.h"

#include <util/generic/algorithm.h>


namespace NDoom {


template <class ChunkedBlobStorage, class Cache>
class TChunkedBlobStorageWithCache: public IChunkedBlobStorage {
    using TChunkedBlobStorage = ChunkedBlobStorage;
    using TCache = Cache;
public:
    TChunkedBlobStorageWithCache() = default;

    TChunkedBlobStorageWithCache(const TChunkedBlobStorage* blobStorage, TCache* cache)
        : BlobStorage_(blobStorage)
        , Cache_(cache)
    {

    }

    TChunkedBlobStorageWithCache(const TChunkedBlobStorage* blobStorage, THolder<TCache>&& cache)
        : BlobStorage_(blobStorage)
        , LocalCache_(std::forward<THolder<TCache>>(cache))
        , Cache_(LocalCache_.Get())
    {

    }

    ui32 Chunks() const override {
        return BlobStorage_->Chunks();
    }

    ui32 ChunkSize(ui32 chunk) const override {
        return BlobStorage_->ChunkSize(chunk);
    }

    TBlob Read(ui32 chunk, ui32 id) const override {
        TBlob result;
        if (!Cache_->TryLoadBlob(chunk, id, &result)) {
            result = BlobStorage_->Read(chunk, id);
        }
        Cache_->StoreBlob(chunk, id, result);
        return result;
    }

    TVector<TBlob> Read(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids) const override
    {
        Y_ENSURE(chunks.size() == ids.size());
        if (chunks.empty()) {
            return TVector<TBlob>();
        }
        TVector<TBlob> result(chunks.size());
        TVector<bool> statuses(chunks.size());
        Cache_->TryLoadBlobs(chunks, ids, statuses, result);
        const size_t fetchSize = CountIf(statuses, [](bool status) { return !status; });
        if (fetchSize != 0) {
            TVector<ui32> fetchChunks(fetchSize);
            TVector<ui32> fetchIds(fetchSize);
            size_t ptr = 0;
            for (size_t i = 0; i < chunks.size(); ++i) {
                if (!statuses[i]) {
                    fetchChunks[ptr] = chunks[i];
                    fetchIds[ptr] = ids[i];
                    ++ptr;
                }
            }
            TVector<TBlob> fetchBlobs = BlobStorage_->Read(fetchChunks, fetchIds);
            ptr = 0;
            for (size_t i = 0; i < chunks.size(); ++i) {
                if (!statuses[i]) {
                    result[i] = std::move(fetchBlobs[ptr++]);
                    statuses[i] = true;
                }
            }
        }
        Cache_->StoreBlobs(chunks, ids, result, statuses);
        return result;
    }

    void Read(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids,
        std::function<void(size_t, TMaybe<TBlob>&&)> callback) const override
    {
        Y_ENSURE(chunks.size() == ids.size());
        if (chunks.empty()) {
            return;
        }
        TVector<TBlob> result(chunks.size());
        TVector<bool> statuses(chunks.size());
        Cache_->TryLoadBlobs(chunks, ids, statuses, result);
        size_t fetchSize = chunks.size();
        for (size_t i = 0; i < chunks.size(); ++i) {
            if (statuses[i]) {
                TBlob blob = result[i];
                callback(i, std::move(blob));
                --fetchSize;
            }
        }

        if (fetchSize != 0) {
            TVector<ui32> fetchChunks(Reserve(fetchSize));
            TVector<ui32> fetchIds(Reserve(fetchSize));
            TVector<ui32> fetchIndices(Reserve(fetchSize));
            for (size_t i = 0; i < chunks.size(); ++i) {
                if (!statuses[i]) {
                    fetchChunks.push_back(chunks[i]);
                    fetchIds.push_back(ids[i]);
                    fetchIndices.push_back(i);
                }
            }
            BlobStorage_->Read(fetchChunks, fetchIds,
                [&](size_t i, TMaybe<TBlob>&& blob) {
                    if (blob) {
                        statuses[fetchIndices[i]] = true;
                        result[fetchIndices[i]] = *blob;
                    }
                    callback(fetchIndices[i], std::move(blob));
                });
        }
        Cache_->StoreBlobs(chunks, ids, result, statuses);
    }

private:
    const TChunkedBlobStorage* BlobStorage_ = nullptr;
    THolder<TCache> LocalCache_;
    mutable TCache* Cache_ = nullptr;
};


} // namespace NDoom
