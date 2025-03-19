#pragma once

#include "direct_io_wad_chunk.h"
#include "wad_chunked_blob_storage.h"

#if defined(_linux_)
#include <kernel/doom/direct_io/async_direct_io_thread_local_reader.h>
#include <library/cpp/threading/thread_local/thread_local.h>

#include <util/system/tls.h>
#endif

#include <util/datetime/base.h>

#include <library/cpp/containers/stack_vector/stack_vec.h>

namespace NDoom {


#if defined(_linux_)

class TDirectAioWadChunkedBlobStorage: public TWadChunkedBlobStorage<TDirectIoWadChunk> {
    using TBase = TWadChunkedBlobStorage<TDirectIoWadChunk>;
public:
    TDirectAioWadChunkedBlobStorage() = default;

    TDirectAioWadChunkedBlobStorage(
        const TArrayRef<const TString>& paths,
        bool lockMemory = false,
        const TString& signature = TString())
        : TBase(paths, lockMemory, signature)
    {
    }

    TDirectAioWadChunkedBlobStorage(TVector<TWadChunk>&& chunks)
        : TBase(std::forward<TVector<TWadChunk>>(chunks))
    {
    }

    static constexpr size_t MaxBatchSize = 16;

    using TBase::TBase;

    TBlob Read(ui32 chunk, ui32 id) const override {
        return TBase::Read(chunk, id);
    }

    TVector<TBlob> Read(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids) const override
    {
        Y_ENSURE(chunks.size() == ids.size());
        if (chunks.empty()) {
            return TVector<TBlob>();
        }
        if (Y_LIKELY(chunks.size() <= MaxBatchSize)) {
            std::array<FHANDLE, MaxBatchSize> fileHandles;
            std::array<ui64, MaxBatchSize> offsets;
            std::array<ui64, MaxBatchSize> sizes;
            return ReadInternal(
                chunks,
                ids,
                TArrayRef<FHANDLE>(fileHandles.data(), chunks.size()),
                TArrayRef<ui64>(offsets.data(), chunks.size()),
                TArrayRef<ui64>(sizes.data(), chunks.size()));
        }
        TVector<FHANDLE> fileHandles(chunks.size());
        TVector<ui64> offsets(chunks.size());
        TVector<ui64> sizes(chunks.size());
        return ReadInternal(chunks, ids, fileHandles, offsets, sizes);
    }

    void ResolveLocations(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids,
        TArrayRef<FHANDLE> fileHandles,
        TArrayRef<ui64> offsets,
        TArrayRef<ui64> sizes) const
    {
        for (size_t i = 0; i < chunks.size(); ++i) {
            Y_ENSURE(ids[i] < Chunk(chunks[i]).Size());
            fileHandles[i] = Chunk(chunks[i]).FileHandle();

            std::pair<ui64, ui64> location = Chunk(chunks[i]).Location(ids[i]);
            offsets[i] = location.first;
            sizes[i] = location.second;
        }
    }

    TAsyncDirectIoThreadLocalReader* ThreadLocalReader() const {
        return ThreadLocalReader_.Get();
    }

    void Read(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids,
        std::function<void(size_t, TMaybe<TBlob>&&)> callback) const override
    {
        TStackVec<FHANDLE, MaxBatchSize> fileHandles(chunks.size());
        TStackVec<ui64, MaxBatchSize> offsets(chunks.size());
        TStackVec<ui64, MaxBatchSize> sizes(chunks.size());
        TStackVec<bool, MaxBatchSize> moved(chunks.size(), false);
        ResolveLocations(chunks, ids, fileHandles, offsets, sizes);
        TAsyncDirectIoThreadLocalReader* reader = ThreadLocalReader();
        for (size_t from = 0; from < chunks.size(); ) {
            size_t batchSize = reader->MaxEvents();
            if (chunks.size() - from < batchSize) {
                batchSize = chunks.size() - from;
            }
            const size_t to = from + batchSize;
            reader->Submit({&fileHandles[from], batchSize}, {&offsets[from], batchSize}, {&sizes[from], batchSize});

            reader->Lock();
            try {
                while (reader->ProcessingRequests() > 0) {
                    reader->Poll();
                    for (size_t i = from; i < to; ++i) {
                        if (reader->IsReady(i - from) && !moved[i]) {
                            callback(i, reader->MoveResult(i - from));
                            moved[i] = true;
                        }
                    }
                }
            } catch(...) {
                //TODO: replace with Cancel()
                reader->DestroyContext();
                reader->Unlock();
                throw;
            }
            reader->Unlock();
            from = to;
        }
    }

private:
    TVector<TBlob> ReadInternal(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids,
        TArrayRef<FHANDLE> fileHandles,
        TArrayRef<ui64> offsets,
        TArrayRef<ui64> sizes) const
    {
        ResolveLocations(chunks, ids, fileHandles, offsets, sizes);
        TVector<TBlob> result(chunks.size());
        TAsyncDirectIoThreadLocalReader* reader = ThreadLocalReader();
        for (size_t from = 0; from < chunks.size(); ) {
            size_t batchSize = reader->MaxEvents();
            if (chunks.size() - from < batchSize) {
                batchSize = chunks.size() - from;
            }
            const size_t to = from + batchSize;
            reader->Submit(fileHandles.Slice(from, batchSize), offsets.Slice(from, batchSize), sizes.Slice(from, batchSize));
            reader->GetAllEvents();
            for (size_t i = from; i < to; ++i) {
                Y_ENSURE(reader->IsReady(i - from));
                result[i] = reader->MoveResult(i - from);
            }
            from = to;
        }
        return result;
    }


    NThreading::TThreadLocalValue<TAsyncDirectIoThreadLocalReader> ThreadLocalReader_;
};


#else

using TDirectAioWadChunkedBlobStorage = TWadChunkedBlobStorage<TDirectIoWadChunk>;

#endif

class TDirectAioWadChunkedBlobStorageWithStatistics: public TDirectAioWadChunkedBlobStorage {
    using TBase = TDirectAioWadChunkedBlobStorage;
public:
    TDirectAioWadChunkedBlobStorageWithStatistics() = default;

    TDirectAioWadChunkedBlobStorageWithStatistics(
        const TArrayRef<const TString>& paths,
        bool lockMemory = false,
        const TString& signature = TString())
        : TBase(paths, lockMemory, signature)
    {
    }

    TDirectAioWadChunkedBlobStorageWithStatistics(TVector<TWadChunk>&& chunks)
        : TBase(std::forward<TVector<TWadChunk>>(chunks))
    {
    }

    TBlob Read(ui32 chunk, ui32 id) const override {
        TInstant start(Now());
        TBlob result = TDirectAioWadChunkedBlobStorage::Read(chunk, id);

        Records.emplace_back(Now() - start, 1);
        return result;
    }

    TVector<TBlob> Read(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids) const override
    {
        TInstant start(Now());
        TVector<TBlob> result = TDirectAioWadChunkedBlobStorage::Read(chunks, ids);
        Records.emplace_back(Now() - start, ids.size());
        return result;
    }

    TVector<std::pair<TDuration, size_t>> TakeRecords() const {
        TVector<std::pair<TDuration, size_t>> result(std::move(Records));
        Records = {};
        return result;
    }
private:
    mutable TVector<std::pair<TDuration, size_t>> Records;
};

template<class... Args>
TDirectAioWadChunkedBlobStorage* CreateDirectAioWadChunkedBlobStorage(bool withStatistics, Args&&... args) {
    if (withStatistics) {
        return new TDirectAioWadChunkedBlobStorageWithStatistics(std::forward<Args>(args)...);
    }
    return new TDirectAioWadChunkedBlobStorage(std::forward<Args>(args)...);
}

} // namespace NDoom
