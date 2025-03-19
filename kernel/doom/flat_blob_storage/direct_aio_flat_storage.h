#pragma once

#include "flat_blob_storage.h"

#if defined(_linux_)

#include <kernel/doom/direct_io/async_direct_io_thread_local_reader.h>
#include <kernel/doom/direct_io/direct_io_file_read_request.h>
#include <library/cpp/threading/thread_local/thread_local.h>

#include <util/system/filemap.h>
#include <util/system/tls.h>

namespace NDoom {

class TDirectAioFlatStorage: public IFlatBlobStorage {
public:
    TDirectAioFlatStorage() = default;

    template<typename... Args>
    TDirectAioFlatStorage(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(TConstArrayRef<TString> files) {
        Reset(TConstArrayRef<TConstArrayRef<TString>>{files});
    }

    void Reset(TConstArrayRef<TConstArrayRef<TString>> files) {
        Files_.clear();
        Files_.resize(files.size());
        for (size_t i = 0; i < files.size(); ++i) {
            Files_[i].resize(files[i].size());
            for (size_t j = 0; j < files[i].size(); ++j) {
                Files_[i][j] = MakeHolder<TFile>(files[i][j], EOpenModeFlag::OpenExisting | EOpenModeFlag::RdOnly | EOpenModeFlag::Direct | EOpenModeFlag::DirectAligned);
                Files_[i][j]->SetDirect();
            }
        }
    }

    using IFlatBlobStorage::Read;

    void Read(
        TConstArrayRef<ui32> chunks,
        TConstArrayRef<ui32> parts,
        TConstArrayRef<ui64> offsets,
        TConstArrayRef<ui64> sizes,
        TArrayRef<TBlob> blobs) const override
    {
        TVector<FHANDLE> handles(parts.size());
        for (size_t i = 0; i < parts.size(); ++i) {
            handles[i] = Files_[chunks[i]][parts[i]]->GetHandle();
        }
        TAsyncDirectIoThreadLocalReader* reader = ThreadLocalReader();
        const size_t maxBatch = reader->MaxEvents();
        for (size_t from = 0; from < parts.size(); from += maxBatch) {
            const size_t batch  = Min(maxBatch, parts.size() - from);
            TConstArrayRef<FHANDLE> batchHandles{handles.data() + from, handles.data() + from + batch};
            reader->Submit(batchHandles, offsets.Slice(from, batch), sizes.Slice(from, batch));
            reader->GetAllEvents();
            for (size_t i = 0; i < batch; ++i) {
                blobs[from + i] = reader->MoveResult(i);
            }
        }
    }

    ui32 Chunks() const override {
        return Files_.size();
    }

    TAsyncDirectIoThreadLocalReader* ThreadLocalReader() const {
        return ThreadLocalReader_.Get();
    }

private:
    TVector<TVector<THolder<TFile>>> Files_;

    NThreading::TThreadLocalValue<TAsyncDirectIoThreadLocalReader> ThreadLocalReader_;
};

} // namespace NDoom

#else

#include "mapped_flat_storage.h"

namespace NDoom {

using TDirectAioFlatStorage = TMappedFlatStorage;

} // namespace NDoom

#endif
