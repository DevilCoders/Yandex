#pragma once

#include "public.h"

#include <cloud/blockstore/libs/common/block_buffer.h>
#include <cloud/blockstore/libs/common/block_range.h>
#include <cloud/blockstore/libs/common/guarded_sglist.h>
#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/kikimr/events.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>
#include <cloud/blockstore/libs/storage/model/channel_data_kind.h>
#include <cloud/blockstore/libs/storage/model/channel_permissions.h>
#include <cloud/blockstore/libs/storage/partition/model/block.h>
#include <cloud/blockstore/libs/storage/partition/model/block_mask.h>
#include <cloud/blockstore/libs/storage/protos/part.pb.h>
#include <cloud/blockstore/libs/storage/protos/volume.pb.h>

#include <cloud/storage/core/libs/tablet/model/partial_blob_id.h>

#include <ydb/core/base/blobstorage.h>

#include <library/cpp/actors/core/actorid.h>
#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <library/cpp/lwtrace/shuttle.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

////////////////////////////////////////////////////////////////////////////////

enum EAddBlobMode
{
    ADD_WRITE_RESULT,
    ADD_FLUSH_RESULT,
    ADD_COMPACTION_RESULT,
};

////////////////////////////////////////////////////////////////////////////////

struct TAddMixedBlob
{
    const TPartialBlobId BlobId;
    const TVector<ui32> Blocks;

    TAddMixedBlob(
            const TPartialBlobId& blobId,
            TVector<ui32> blocks)
        : BlobId(blobId)
        , Blocks(std::move(blocks))
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct TAddMergedBlob
{
    const TPartialBlobId BlobId;
    const TBlockRange32 BlockRange;
    const TBlockMask HoleMask;
    const TBlockMask SkipMask;

    TAddMergedBlob(
            const TPartialBlobId& blobId,
            const TBlockRange32& blockRange,
            const TBlockMask& holeMask,
            const TBlockMask& skipMask)
        : BlobId(blobId)
        , BlockRange(blockRange)
        , HoleMask(holeMask)
        , SkipMask(skipMask)
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct TAddFreshBlob
{
    const TPartialBlobId BlobId;
    const TVector<TBlock> Blocks;

    TAddFreshBlob(
            const TPartialBlobId& blobId,
            TVector<TBlock> blocks)
        : BlobId(blobId)
        , Blocks(std::move(blocks))
    {}
};

struct TWriteFreshBlocksRequest
{
    TBlockRange32 BlockRange;
    IWriteBlocksHandlerPtr WriteHandler;

    TWriteFreshBlocksRequest(
            TBlockRange32 blockRange,
            IWriteBlocksHandlerPtr writeHandler)
        : BlockRange(blockRange)
        , WriteHandler(std::move(writeHandler))
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct TAffectedBlob
{
    TVector<ui16> Offsets;
    TMaybe<TBlockMask> BlockMask;
    TVector<ui32> AffectedBlockIndices;
};

using TAffectedBlobs = THashMap<TPartialBlobId, TAffectedBlob, TPartialBlobIdHash>;

////////////////////////////////////////////////////////////////////////////////

struct TAffectedBlock
{
    ui32 BlockIndex = 0;
    ui64 CommitId = 0;
};

using TAffectedBlocks = TVector<TAffectedBlock>;

////////////////////////////////////////////////////////////////////////////////

struct TMergedBlobCompactionInfo
{
    const ui32 BlobsSkippedByCompaction = 0;
    const ui32 BlocksSkippedByCompaction = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TFlushedCommitId
{
    ui64 CommitId;
    ui32 BlockCount;

    TFlushedCommitId(ui64 commitId, ui32 blockCount)
        : CommitId(commitId)
        , BlockCount(blockCount)
    {}
};

using TFlushedCommitIds = TVector<TFlushedCommitId>;

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_PARTITION_REQUESTS_PRIVATE(xxx, ...)                        \
    xxx(ReadBlob,                  __VA_ARGS__)                                \
    xxx(WriteBlob,                 __VA_ARGS__)                                \
    xxx(AddBlobs,                  __VA_ARGS__)                                \
    xxx(AddFreshBlocks,            __VA_ARGS__)                                \
    xxx(Flush,                     __VA_ARGS__)                                \
    xxx(Compaction,                __VA_ARGS__)                                \
    xxx(MetadataRebuildUsedBlocks, __VA_ARGS__)                                \
    xxx(MetadataRebuildBlockCount, __VA_ARGS__)                                \
    xxx(Cleanup,                   __VA_ARGS__)                                \
    xxx(CollectGarbage,            __VA_ARGS__)                                \
    xxx(AddGarbage,                __VA_ARGS__)                                \
    xxx(DeleteGarbage,             __VA_ARGS__)                                \
// BLOCKSTORE_PARTITION_REQUESTS_PRIVATE

////////////////////////////////////////////////////////////////////////////////

struct TReadBlocksRequest
{
    NKikimr::TLogoBlobID BlobId;
    NActors::TActorId BSProxy;
    ui16 BlobOffset;
    ui32 BlockIndex;
    ui32 GroupId;

    TReadBlocksRequest(
            const NKikimr::TLogoBlobID& blobId,
            NActors::TActorId proxy,
            ui16 blobOffset,
            ui32 blockIndex,
            ui32 groupId)
        : BlobId(blobId)
        , BSProxy(proxy)
        , BlobOffset(blobOffset)
        , BlockIndex(blockIndex)
        , GroupId(groupId)
    {}
};

using TReadBlocksRequests = TVector<TReadBlocksRequest>;

////////////////////////////////////////////////////////////////////////////////

struct TBlockCountRebuildState
{
    ui64 MixedBlocks = 0;
    ui64 MergedBlocks = 0;

    ui64 InitialMixedBlocks = 0;
    ui64 InitialMergedBlocks = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TEvPartitionPrivate
{
    //
    // ReadBlob
    //

    struct TReadBlobRequest
    {
        NKikimr::TLogoBlobID BlobId;
        NActors::TActorId Proxy;
        TVector<ui16> BlobOffsets;
        TGuardedSgList Sglist;
        ui32 GroupId = 0;
        bool Async = false;
        TInstant Deadline;

        TReadBlobRequest() = default;

        TReadBlobRequest(
                const NKikimr::TLogoBlobID& blobId,
                NActors::TActorId proxy,
                TVector<ui16> blobOffsets,
                TGuardedSgList sglist,
                ui32 groupId,
                bool async = false,
                TInstant deadline = TInstant::Max())
            : BlobId(blobId)
            , Proxy(proxy)
            , BlobOffsets(std::move(blobOffsets))
            , Sglist(std::move(sglist))
            , GroupId(groupId)
            , Async(async)
            , Deadline(deadline)
        {}
    };

    struct TReadBlobResponse
    {
        ui64 ExecCycles = 0;

        TReadBlobResponse() = default;
    };

    struct TReadBlobCompleted
    {
        NKikimr::TLogoBlobID BlobId;
        ui32 BytesCount = 0;
        TDuration RequestTime;
        ui32 GroupId = 0;

        TReadBlobCompleted() = default;

        TReadBlobCompleted(
                const NKikimr::TLogoBlobID& blobId,
                ui32 bytesCount,
                TDuration requestTime,
                ui32 groupId)
            : BlobId(blobId)
            , BytesCount(bytesCount)
            , RequestTime(requestTime)
            , GroupId(groupId)
        {}
    };

    //
    // WriteBlob
    //

    struct TWriteBlobRequest
    {
        NActors::TActorId Proxy;

        TPartialBlobId BlobId;
        std::variant<TGuardedSgList, TString> Data;

        bool Async = false;
        TInstant Deadline;

        TWriteBlobRequest() = default;

        template <typename TData>
        TWriteBlobRequest(
                TPartialBlobId blobId,
                TData data,
                bool async = false,
                TInstant deadline = TInstant::Max())
            : BlobId(blobId)
            , Data(std::move(data))
            , Async(async)
            , Deadline(deadline)
        {}
    };

    struct TWriteBlobResponse
    {
        ui64 ExecCycles = 0;
    };

    struct TWriteBlobCompleted
    {
        TPartialBlobId BlobId;
        NKikimr::TStorageStatusFlags StorageStatusFlags;
        double ApproximateFreeSpaceShare = 0;
        TDuration RequestTime;

        TWriteBlobCompleted() = default;

        TWriteBlobCompleted(
                const TPartialBlobId& blobId,
                NKikimr::TStorageStatusFlags storageStatusFlags,
                double approximateFreeSpaceShare,
                TDuration requestTime)
            : BlobId(blobId)
            , StorageStatusFlags(storageStatusFlags)
            , ApproximateFreeSpaceShare(approximateFreeSpaceShare)
            , RequestTime(requestTime)
        {}
    };

    //
    // AddBlobs
    //

    struct TAddBlobsRequest
    {
        ui64 CommitId = 0;
        TVector<TAddMixedBlob> MixedBlobs;
        TVector<TAddMergedBlob> MergedBlobs;
        TVector<TAddFreshBlob> FreshBlobs;
        EAddBlobMode Mode = ADD_WRITE_RESULT;

        // compaction
        TAffectedBlobs AffectedBlobs;
        TAffectedBlocks AffectedBlocks;
        TVector<TMergedBlobCompactionInfo> MergedBlobCompactionInfos;

        TAddBlobsRequest() = default;

        TAddBlobsRequest(
                ui64 commitId,
                TVector<TAddMixedBlob> mixedBlobs,
                TVector<TAddMergedBlob> mergedBlobs,
                TVector<TAddFreshBlob> freshBlobs,
                EAddBlobMode mode,
                TAffectedBlobs affectedBlobs = {},
                TAffectedBlocks affectedBlocks = {},
                TVector<TMergedBlobCompactionInfo> mergedBlobCompactionInfos = {})
            : CommitId(commitId)
            , MixedBlobs(std::move(mixedBlobs))
            , MergedBlobs(std::move(mergedBlobs))
            , FreshBlobs(std::move(freshBlobs))
            , Mode(mode)
            , AffectedBlobs(std::move(affectedBlobs))
            , AffectedBlocks(std::move(affectedBlocks))
            , MergedBlobCompactionInfos(std::move(mergedBlobCompactionInfos))
        {}
    };

    struct TAddBlobsResponse
    {
        ui64 ExecCycles = 0;
    };

    //
    // AddFreshBlocks
    //

    struct TAddFreshBlocksRequest
    {
        ui64 CommitId;
        ui64 BlobSize;
        TVector<TBlockRange32> BlockRanges;
        TVector<IWriteBlocksHandlerPtr> WriteHandlers;

        TAddFreshBlocksRequest(
                ui64 commitId,
                ui64 blobSize,
                TVector<TBlockRange32> blockRanges,
                TVector<IWriteBlocksHandlerPtr> writeHandlers)
            : CommitId(commitId)
            , BlobSize(blobSize)
            , BlockRanges(std::move(blockRanges))
            , WriteHandlers(std::move(writeHandlers))
        {}
    };

    struct TAddFreshBlocksResponse
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
    // Compaction
    //

    enum ECompactionMode
    {
        RangeCompaction,
        GarbageCompaction
    };

    struct TCompactionRequest
    {
        ECompactionMode Mode = RangeCompaction;
        TMaybe<ui32> BlockIndex;
        bool ForceFullCompaction = false;

        TCompactionRequest() = default;

        TCompactionRequest(ui32 blockIndex, bool forceFullCompaction)
            : Mode(RangeCompaction)
            , BlockIndex(blockIndex)
            , ForceFullCompaction(forceFullCompaction)
        {}

        TCompactionRequest(ECompactionMode mode)
            : Mode(mode)
        {}
    };

    struct TCompactionResponse
    {
    };

    //
    // MetadataRebuildUsedBlocks
    //

    struct TMetadataRebuildUsedBlocksRequest
    {
        ui32 Begin = 0;
        ui32 End = 0;

        TMetadataRebuildUsedBlocksRequest() = default;

        TMetadataRebuildUsedBlocksRequest(ui32 begin, ui32 end)
            : Begin(begin)
            , End(end)
        {}
    };

    struct TMetadataRebuildUsedBlocksResponse
    {
    };

    //
    // MetadataRebuildBlockCount
    //

    struct TMetadataRebuildBlockCountRequest
    {
        const TPartialBlobId BlobId;
        const ui32 Count = 0;
        const TPartialBlobId FinalBlobId;

        const TBlockCountRebuildState RebuildState;

        TMetadataRebuildBlockCountRequest(
                TPartialBlobId blobId,
                ui32 count,
                TPartialBlobId finalBlobId,
                const TBlockCountRebuildState& rebuildState)
            : BlobId(blobId)
            , Count(count)
            , FinalBlobId(finalBlobId)
            , RebuildState(rebuildState)
        {}
    };

    struct TMetadataRebuildBlockCountResponse
    {
        TPartialBlobId LastReadBlobId;
        TBlockCountRebuildState RebuildState;

        TMetadataRebuildBlockCountResponse() = default;

        TMetadataRebuildBlockCountResponse(
                TPartialBlobId lastReadBlobId,
                const TBlockCountRebuildState& rebuildState)
            : LastReadBlobId(lastReadBlobId)
            , RebuildState(rebuildState)
        {}
    };

    //
    // Cleanup
    //

    struct TCleanupRequest
    {
    };

    struct TCleanupResponse
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
    // AddGarbage
    //

    struct TAddGarbageRequest
    {
        TVector<TPartialBlobId> BlobIds;

        TAddGarbageRequest() = default;

        TAddGarbageRequest(TVector<TPartialBlobId> blobIds)
            : BlobIds(std::move(blobIds))
        {}
    };

    struct TAddGarbageResponse
    {
    };

    //
    // DeleteGarbage
    //

    struct TDeleteGarbageRequest
    {
        ui64 CommitId = 0;
        TVector<TPartialBlobId> NewBlobs;
        TVector<TPartialBlobId> GarbageBlobs;

        TDeleteGarbageRequest() = default;

        TDeleteGarbageRequest(
                ui64 commitId,
                TVector<TPartialBlobId> newBlobs,
                TVector<TPartialBlobId> garbageBlobs)
            : CommitId(commitId)
            , NewBlobs(std::move(newBlobs))
            , GarbageBlobs(std::move(garbageBlobs))
        {}
    };

    struct TDeleteGarbageResponse
    {
        ui64 ExecCycles = 0;
    };

    //
    // OperationCompleted
    //

    struct TOperationCompleted
    {
        NProto::TPartitionStats Stats;

        ui64 TotalCycles = 0;
        ui64 ExecCycles = 0;

        ui64 CommitId = 0;

        TVector<TBlockRange64> AffectedRanges;
        TVector<IProfileLog::TBlockInfo> AffectedBlockInfos;
    };

    //
    // ReadBlocksCompleted
    //

    struct TReadBlocksCompleted
        : TOperationCompleted
    {
        struct TCompactionRangeReadStats
        {
            ui32 BlockIndex;
            ui32 BlobCount;
            ui32 BlockCount;

            TCompactionRangeReadStats(
                    ui32 blockIndex,
                    ui32 blobCount,
                    ui32 blockCount)
                : BlockIndex(blockIndex)
                , BlobCount(blobCount)
                , BlockCount(blockCount)
            {
            }
        };

        TStackVec<TCompactionRangeReadStats, 2> ReadStats;
    };

    //
    // WriteBlocksCompleted
    //

    struct TWriteBlocksCompleted
        : TOperationCompleted
    {
        bool CollectGarbageBarrierAcquired;

        TWriteBlocksCompleted(
                bool collectGarbageBarrierAcquired)
            : CollectGarbageBarrierAcquired(collectGarbageBarrierAcquired)
        {
        }
    };

    //
    // FlushCompleted
    //

    struct TFlushCompleted
        : TOperationCompleted
    {
        ui32 FlushedFreshBlobCount;
        ui64 FlushedFreshBlobByteCount;
        TFlushedCommitIds FlushedCommitIdsFromChannel;

        TFlushCompleted(
                ui32 flushedFreshBlobCount,
                ui64 flushedFreshBlobByteCount,
                TFlushedCommitIds flushedCommitIdsFromChannel)
            : FlushedFreshBlobCount(flushedFreshBlobCount)
            , FlushedFreshBlobByteCount(flushedFreshBlobByteCount)
            , FlushedCommitIdsFromChannel(std::move(flushedCommitIdsFromChannel))
        {
        }
    };

    //
    // ForcedCompactionCompleted
    //

    struct TForcedCompactionCompleted
    {
    };

    //
    // MetadataRebuildCompleted
    //

    struct TMetadataRebuildCompleted
    {
    };

    //
    // LoadStateCompleted
    //

    struct TLoadStateCompleted
        : TOperationCompleted
    {
        TVector<NProto::TUsedBlockData> UsedBlocksFromBaseDisk;

        TLoadStateCompleted() = default;

        TLoadStateCompleted(
                const TVector<NProto::TUsedBlockData>& usedBlocksFromBaseDisk)
            : UsedBlocksFromBaseDisk(usedBlocksFromBaseDisk)
        {
        }

    };

    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TBlockStorePrivateEvents::PARTITION_START,

        BLOCKSTORE_PARTITION_REQUESTS_PRIVATE(BLOCKSTORE_DECLARE_EVENT_IDS)

        EvUpdateCounters,
        EvUpdateYellowState,
        EvSendBackpressureReport,
        EvProcessWriteQueue,

        EvReadBlobCompleted,
        EvWriteBlobCompleted,
        EvReadBlocksCompleted,
        EvWriteBlocksCompleted,
        EvZeroBlocksCompleted,
        EvFlushCompleted,
        EvCompactionCompleted,
        EvCollectGarbageCompleted,
        EvForcedCompactionCompleted,
        EvMetadataRebuildCompleted,
        EvLoadStateCompleted,
        EvGetChangedBlocksCompleted,

        EvEnd
    };

    static_assert(EvEnd < (int)TBlockStorePrivateEvents::PARTITION_END,
        "EvEnd expected to be < TBlockStorePrivateEvents::PARTITION_END");

    BLOCKSTORE_PARTITION_REQUESTS_PRIVATE(BLOCKSTORE_DECLARE_EVENTS)

    using TEvUpdateCounters = TRequestEvent<TEmpty, EvUpdateCounters>;
    using TEvUpdateYellowState = TRequestEvent<TEmpty, EvUpdateYellowState>;
    using TEvSendBackpressureReport = TRequestEvent<TEmpty, EvSendBackpressureReport>;
    using TEvProcessWriteQueue = TRequestEvent<TEmpty, EvProcessWriteQueue>;

    using TEvReadBlobCompleted = TResponseEvent<TReadBlobCompleted, EvReadBlobCompleted>;
    using TEvWriteBlobCompleted = TResponseEvent<TWriteBlobCompleted, EvWriteBlobCompleted>;
    using TEvReadBlocksCompleted = TResponseEvent<TReadBlocksCompleted, EvReadBlocksCompleted>;
    using TEvWriteBlocksCompleted = TResponseEvent<TWriteBlocksCompleted, EvWriteBlocksCompleted>;
    using TEvZeroBlocksCompleted = TResponseEvent<TOperationCompleted, EvZeroBlocksCompleted>;
    using TEvFlushCompleted = TResponseEvent<TFlushCompleted, EvFlushCompleted>;
    using TEvCompactionCompleted = TResponseEvent<TOperationCompleted, EvCompactionCompleted>;
    using TEvCollectGarbageCompleted = TResponseEvent<TOperationCompleted, EvCollectGarbageCompleted>;
    using TEvForcedCompactionCompleted = TResponseEvent<TForcedCompactionCompleted, EvForcedCompactionCompleted>;
    using TEvMetadataRebuildCompleted = TResponseEvent<TOperationCompleted, EvMetadataRebuildCompleted>;
    using TEvLoadStateCompleted = TResponseEvent<TLoadStateCompleted, EvLoadStateCompleted>;
    using TEvGetChangedBlocksCompleted = TResponseEvent<TOperationCompleted, EvGetChangedBlocksCompleted>;
};

}   // namespace NCloud::NBlockStore::NStorage::NPartition
