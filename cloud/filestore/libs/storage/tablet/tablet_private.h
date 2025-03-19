#pragma once

#include "public.h"

#include <cloud/filestore/libs/service/error.h>
#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/storage/api/components.h>
#include <cloud/filestore/libs/storage/api/events.h>
#include <cloud/filestore/libs/storage/tablet/model/blob.h>
#include <cloud/filestore/libs/storage/tablet/model/block.h>

#include <ydb/core/base/blobstorage.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_TABLET_REQUESTS_PRIVATE(xxx, ...)                            \
    xxx(CleanupSessions,                        __VA_ARGS__)                   \
    xxx(WriteBatch,                             __VA_ARGS__)                   \
    xxx(ReadBlob,                               __VA_ARGS__)                   \
    xxx(WriteBlob,                              __VA_ARGS__)                   \
    xxx(AddBlob,                                __VA_ARGS__)                   \
    xxx(Flush,                                  __VA_ARGS__)                   \
    xxx(FlushBytes,                             __VA_ARGS__)                   \
    xxx(Cleanup,                                __VA_ARGS__)                   \
    xxx(Compaction,                             __VA_ARGS__)                   \
    xxx(CollectGarbage,                         __VA_ARGS__)                   \
    xxx(DeleteGarbage,                          __VA_ARGS__)                   \
    xxx(DeleteCheckpoint,                       __VA_ARGS__)                   \
// FILESTORE_TABLET_REQUESTS_PRIVATE

////////////////////////////////////////////////////////////////////////////////

struct TReadBlob
{
    struct TBlock
    {
        ui32 BlobOffset;
        ui32 BlockOffset;

        TBlock(ui32 blobOffset, ui32 blockOffset)
            : BlobOffset(blobOffset)
            , BlockOffset(blockOffset)
        {}
    };

    TPartialBlobId BlobId;
    TVector<TBlock> Blocks;

    TInstant Deadline = TInstant::Max();
    bool Async = false;

    TReadBlob(const TPartialBlobId& blobId, TVector<TBlock> blocks)
        : BlobId(blobId)
        , Blocks(std::move(blocks))
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct TWriteBlob
{
    TPartialBlobId BlobId;
    TString BlobContent;

    TInstant Deadline = TInstant::Max();
    bool Async = false;

    TWriteBlob(const TPartialBlobId& blobId, TString blobContent)
        : BlobId(blobId)
        , BlobContent(std::move(blobContent))
    {}
};

////////////////////////////////////////////////////////////////////////////////

enum class EAddBlobMode
{
    Write,
    Flush,
    FlushBytes,
    Compaction,
};

////////////////////////////////////////////////////////////////////////////////

enum class EDeleteCheckpointMode
{
    MarkCheckpointDeleted,
    RemoveCheckpointNodes,
    RemoveCheckpointBlobs,
    RemoveCheckpoint,
};

////////////////////////////////////////////////////////////////////////////////

struct TWriteRange
{
    ui64 NodeId = InvalidNodeId;
    ui64 MaxOffset = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TEvIndexTabletPrivate
{
    //
    // CleanupSessions
    //

    struct TCleanupSessionsRequest
    {
    };

    struct TCleanupSessionsResponse
    {
    };

    //
    // WriteBatch
    //

    struct TWriteBatchRequest
    {
    };

    struct TWriteBatchResponse
    {
    };

    //
    // ReadBlob
    //

    struct TReadBlobRequest
    {
        IBlockBufferPtr Buffer;
        TVector<TReadBlob> Blobs;
    };

    struct TReadBlobResponse
    {
    };

    //
    // WriteBlob
    //

    struct TWriteBlobRequest
    {
        TVector<TWriteBlob> Blobs;
    };

    struct TWriteBlobResponse
    {
    };

    struct TWriteBlobCompleted
    {
        struct TWriteRequestResult
        {
            NKikimr::TLogoBlobID BlobId;
            NKikimr::TStorageStatusFlags StorageStatusFlags;
            double ApproximateFreeSpaceShare = 0;
        };

        TVector<TWriteRequestResult> Results;
    };

    //
    // AddBlob
    //

    struct TAddBlobRequest
    {
        EAddBlobMode Mode;
        TVector<TMixedBlobMeta> SrcBlobs;
        TVector<TBlock> SrcBlocks;
        TVector<TMixedBlobMeta> MixedBlobs;
        TVector<TMergedBlobMeta> MergedBlobs;
        TVector<TWriteRange> WriteRanges;
    };

    struct TAddBlobResponse
    {
    };

    //
    // Flush
    //

    struct TFlushRequest
    {
    };

    struct TFlushResponse
    {
    };

    //
    // FlushBytes
    //

    struct TFlushBytesRequest
    {
    };

    struct TFlushBytesResponse
    {
    };

    struct TFlushBytesCompleted
    {
        ui64 CollectCommitId = 0;
        ui64 ChunkId = 0;
        TCallContextPtr CallContext;
    };

    //
    // Cleanup
    //

    struct TCleanupRequest
    {
        ui32 RangeId;

        TCleanupRequest(ui32 rangeId)
            : RangeId(rangeId)
        {}
    };

    struct TCleanupResponse
    {
    };

    //
    // Compaction
    //

    struct TCompactionRequest
    {
        ui32 RangeId;

        TCompactionRequest(ui32 rangeId)
            : RangeId(rangeId)
        {}
    };

    struct TCompactionResponse
    {
    };

    //
    // CollectGarbage
    //

    struct TCollectGarbageRequest
    {
    };

    struct TCollectGarbageResponse
    {
    };

    //
    // DeleteGarbage
    //

    struct TDeleteGarbageRequest
    {
        ui64 CollectCommitId;
        TVector<TPartialBlobId> NewBlobs;
        TVector<TPartialBlobId> GarbageBlobs;

        TDeleteGarbageRequest(
                ui64 collectCommitId,
                TVector<TPartialBlobId> newBlobs,
                TVector<TPartialBlobId> garbageBlobs)
            : CollectCommitId(collectCommitId)
            , NewBlobs(std::move(newBlobs))
            , GarbageBlobs(std::move(garbageBlobs))
        {}
    };

    struct TDeleteGarbageResponse
    {
    };

    //
    // DeleteCheckpoint
    //

    struct TDeleteCheckpointRequest
    {
        TString CheckpointId;
        EDeleteCheckpointMode Mode;

        TDeleteCheckpointRequest(
                TString checkpointId,
                EDeleteCheckpointMode mode)
            : CheckpointId(std::move(checkpointId))
            , Mode(mode)
        {}
    };

    struct TDeleteCheckpointResponse
    {
    };

    //
    // Operation completion
    //

    struct TOperationCompleted
    {
        ui64 CommitId = 0;
    };

    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TFileStoreEventsPrivate::TABLET_START,

        FILESTORE_TABLET_REQUESTS_PRIVATE(FILESTORE_DECLARE_EVENT_IDS)

        EvUpdateCounters,

        EvCleanupSessionsCompleted,
        EvReadDataCompleted,
        EvWriteDataCompleted,
        EvWriteBatchCompleted,
        EvReadBlobCompleted,
        EvWriteBlobCompleted,
        EvFlushCompleted,
        EvFlushBytesCompleted,
        EvCompactionCompleted,
        EvCollectGarbageCompleted,
        EvDestroyCheckpointCompleted,

        EvEnd
    };

    static_assert(EvEnd < (int)TFileStoreEventsPrivate::TABLET_END,
        "EvEnd expected to be < TFileStoreEventsPrivate::TABLET_END");

    FILESTORE_TABLET_REQUESTS_PRIVATE(FILESTORE_DECLARE_EVENTS)

    using TEvUpdateCounters = TRequestEvent<TEmpty, EvUpdateCounters>;

    using TEvCleanupSessionsCompleted = TResponseEvent<TEmpty, EvCleanupSessionsCompleted>;
    using TEvReadDataCompleted = TResponseEvent<TOperationCompleted, EvReadDataCompleted>;
    using TEvWriteDataCompleted = TResponseEvent<TOperationCompleted, EvWriteDataCompleted>;
    using TEvWriteBatchCompleted = TResponseEvent<TOperationCompleted, EvWriteBatchCompleted>;
    using TEvReadBlobCompleted = TResponseEvent<TEmpty, EvReadBlobCompleted>;
    using TEvWriteBlobCompleted = TResponseEvent<TWriteBlobCompleted, EvWriteBlobCompleted>;
    using TEvFlushCompleted = TResponseEvent<TOperationCompleted, EvFlushCompleted>;
    using TEvFlushBytesCompleted = TResponseEvent<TFlushBytesCompleted, EvFlushBytesCompleted>;
    using TEvCompactionCompleted = TResponseEvent<TOperationCompleted, EvCompactionCompleted>;
    using TEvCollectGarbageCompleted = TResponseEvent<TOperationCompleted, EvCollectGarbageCompleted>;
    using TEvDestroyCheckpointCompleted = TResponseEvent<TEmpty, EvDestroyCheckpointCompleted>;
};

}   // namespace NCloud::NFileStore::NStorage
