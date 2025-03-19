#pragma once

#include "chunked_blob_storage.h"


namespace NDoom {


template <class WadChunk>
class TWadChunkedBlobStorage: public IChunkedBlobStorage {
public:
    using TWadChunk = WadChunk;

    TWadChunkedBlobStorage() = default;

    TWadChunkedBlobStorage(
        const TArrayRef<const TString>& paths,
        bool lockMemory = false,
        const TString& signature = TString())
    {
        Chunks_.reserve(paths.size());
        for (size_t i = 0; i < paths.size(); ++i) {
            Chunks_.emplace_back(paths[i], lockMemory, signature);
        }
    }

    TWadChunkedBlobStorage(TVector<TWadChunk>&& chunks)
        : Chunks_(std::forward<TVector<TWadChunk>>(chunks))
    {

    }

    ui32 Chunks() const override {
        return Chunks_.size();
    }

    ui32 ChunkSize(ui32 chunk) const override {
        return Chunk(chunk).Size();
    }

    using IChunkedBlobStorage::Read;

    TBlob Read(ui32 chunk, ui32 id) const override {
        Y_ENSURE(id < Chunk(chunk).Size());
        return Chunk(chunk).Read(id);
    }

    TVector<TBlob> Read(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids) const override
    {
        Y_ENSURE(chunks.size() == ids.size());
        TVector<TBlob> result(chunks.size());
        for (size_t i = 0; i < chunks.size(); ++i) {
            Y_ENSURE(ids[i] < Chunk(chunks[i]).Size());
            result[i] = Chunk(chunks[i]).Read(ids[i]);
        }
        return result;
    }

    const TWadChunk& Chunk(ui32 chunk) const {
        Y_ENSURE(chunk < Chunks_.size());
        return Chunks_[chunk];
    }

private:
    TVector<TWadChunk> Chunks_;
};


} // namespace NDoom
