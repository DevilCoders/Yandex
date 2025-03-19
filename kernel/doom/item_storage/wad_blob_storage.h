#pragma once

#include "blob_storage.h"
#include "item_chunk_mapper.h"

#include <kernel/doom/blob_storage/chunked_blob_storage.h>

#include <utility>

namespace NDoom::NItemStorage {

class TChunkedWadItemBlobStorageBuilder {
public:
    TChunkedWadItemBlobStorageBuilder& SetMapper(THolder<IItemChunkMapper> mapper) {
        Mapper_ = std::move(mapper);
        return *this;
    }

    TChunkedWadItemBlobStorageBuilder& SetBlobStorage(const IChunkedBlobStorage* blobStorage) {
        BlobStorage_ = blobStorage;
        return *this;
    }

    TChunkedWadItemBlobStorageBuilder& SetBlobStorage(THolder<IChunkedBlobStorage> blobStorage) {
        LocalBlobStorage_ = std::move(blobStorage);
        BlobStorage_ = LocalBlobStorage_.Get();
        return *this;
    }

    TChunkedWadItemBlobStorageBuilder& SetWadCommons(TConstArrayRef<TMegaWadCommon> commons) {
        WadCommons_ = commons;
        return *this;
    }

protected:
    void VerifyIsFinished() {
        Y_ENSURE(Mapper_);
        Y_ENSURE(BlobStorage_);
        Y_ENSURE(WadCommons_);
    }

protected:
    THolder<IItemChunkMapper> Mapper_;
    THolder<IChunkedBlobStorage> LocalBlobStorage_;
    const IChunkedBlobStorage* BlobStorage_ = nullptr;
    TConstArrayRef<TMegaWadCommon> WadCommons_;
};

class TChunkedWadItemBlobStorage : public IBlobStorage, private TChunkedWadItemBlobStorageBuilder {
public:
    TChunkedWadItemBlobStorage(TChunkedWadItemBlobStorageBuilder& builder)
        : TChunkedWadItemBlobStorageBuilder{ std::move(builder) }
    {
        VerifyIsFinished();
    }

    void Load(NPrivate::TItemLumpsRequest& req, std::function<void(size_t item, TStatusOr<TItemBlob> blob)> callback) override {
        req.ForEachItem([&](size_t i, const TItemId& item) {
            TMaybe<TItemChunk> itemChunk = Mapper_->GetItemChunk(item);
            if (!itemChunk) {
                callback(i, StatusErr(EStatusCode::NotFound, "Unknown key"));
                return;
            }

            if (req.ItemTypeRequest(item.ItemType).Lumps.empty()) {
                callback(i, TItemBlob{
                    .Chunk = itemChunk->Chunk,
                    .Blob = Nothing(),
                });
                return;
            }

            const ui32 chunk = itemChunk->Chunk.Id;
            const TMegaWadCommon& chunkCommon = WadCommons_[chunk];
            TBlob blob = BlobStorage_->Read(chunk, chunkCommon.Info().FirstDoc + itemChunk->Index);

            callback(i, TItemBlob{
                .Chunk = itemChunk->Chunk,
                .Blob = std::move(blob),
            });
        });
    }
};

} // namespace NDoom::NItemStorage
