#pragma once

#include "public.h"

#include "part_database.h"
#include "part_schema.h"

#include <cloud/blockstore/libs/common/block_range.h>
#include <cloud/blockstore/libs/common/compressed_bitmap.h>
#include <cloud/blockstore/libs/storage/api/partition.h>
#include <cloud/blockstore/libs/storage/core/compaction_map.h>
#include <cloud/blockstore/libs/storage/core/request_buffer.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>
#include <cloud/blockstore/libs/storage/core/ring_buffer.h>
#include <cloud/blockstore/libs/storage/core/write_buffer_request.h>
#include <cloud/blockstore/libs/storage/model/channel_data_kind.h>
#include <cloud/blockstore/libs/storage/model/channel_permissions.h>
#include <cloud/blockstore/libs/storage/partition/model/block_index.h>
#include <cloud/blockstore/libs/storage/partition/model/checkpoint.h>
#include <cloud/blockstore/libs/storage/partition/model/cleanup_queue.h>
#include <cloud/blockstore/libs/storage/partition/model/commit_queue.h>
#include <cloud/blockstore/libs/storage/partition/model/garbage_queue.h>
#include <cloud/blockstore/libs/storage/partition/model/mixed_index_cache.h>
#include <cloud/blockstore/libs/storage/partition/model/operation_status.h>
#include <cloud/blockstore/libs/storage/protos/part.pb.h>

#include <cloud/storage/core/libs/tablet/gc_logic.h>

#include <library/cpp/json/json_value.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

#include <list>
#include <utility>

namespace NCloud::NBlockStore::NStorage::NPartition {

////////////////////////////////////////////////////////////////////////////////

// There is also FreshBlocksCount proto counter. We have to split it into
// FreshBlocksFromDb (we call it FreshBlocks for compatibility) and
// FreshBlocksFromChannel to support fresh channel
// write requests, since there is no Tx on WriteFreshBlock to channel.

#define BLOCKSTORE_PARTITION_PROTO_COUNTERS(xxx)                               \
    xxx(MixedBlocksCount)                                                      \
    xxx(MergedBlocksCount)                                                     \
    xxx(MixedBlobsCount)                                                       \
    xxx(MergedBlobsCount)                                                      \
    xxx(UsedBlocksCount)                                                       \
    xxx(LogicalUsedBlocksCount)                                                \
// BLOCKSTORE_PARTITION_PROTO_COUNTERS

////////////////////////////////////////////////////////////////////////////////

struct TOperationState
{
    EOperationStatus Status = EOperationStatus::Idle;
    TInstant Timestamp;

    void SetStatus(EOperationStatus status)
    {
        Status = status;
        Timestamp = TInstant::Now();
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TForcedCompactionState
{
    bool IsRunning = false;
    ui32 Progress = 0;
    ui32 RangesCount = 0;
    TString OperationId;
};

////////////////////////////////////////////////////////////////////////////////

enum class EMetadataRebuildType
{
    NoOperation,
    UsedBlocks,
    BlockCount
};

////////////////////////////////////////////////////////////////////////////////

struct TUsedBlocksProgress
{
    ui64 BlocksProcessed = 0;
    ui64 BlockCount = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TBlockCountProgress
{
    ui64 TotalBlobs = 0;
    ui32 BlobsProcessed = 0;

    ui64 MixedBlocks = 0;
    ui64 MergedBlocks = 0;

    ui64 LastCommitId = 0;

    TBlockCountProgress() = default;

    TBlockCountProgress(ui64 totalBlobs)
        : TotalBlobs(totalBlobs)
    {
    }

    void UpdateProgress(ui32 blobsRead, ui64 mixedBlocks, ui64 mergedBlocks)
    {
        BlobsProcessed += blobsRead;

        MixedBlocks += mixedBlocks;
        MergedBlocks += mergedBlocks;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TMedatadataRebuildProgress
{
    ui64 Processed = 0;
    ui64 Total = 0;
    bool IsCompleted = false;
};

////////////////////////////////////////////////////////////////////////////////

struct TMetadataRebuildState
{
    bool Started = false;
    EMetadataRebuildType MetadataType = EMetadataRebuildType::NoOperation;

    ui64 Total = 0;
    ui64 Processed = 0;

    bool IsStarted() const
    {
        return Started;
    }

    EMetadataRebuildType GetType() const
    {
        return MetadataType;
    }

    void StartRebuildUsedBlocks(ui64 totalBlocks)
    {
        Started = true;
        MetadataType = EMetadataRebuildType::UsedBlocks;
        Total = totalBlocks;
        Processed = 0;
    }

    void StartRebuildBlockCount(ui64 totalMixedBlobs, ui64 totalMergedBlobs)
    {
        Started = true;
        MetadataType = EMetadataRebuildType::BlockCount;
        Total = totalMixedBlobs + totalMergedBlobs;
        Processed = 0;
    }

    TMedatadataRebuildProgress GetProgress() const
    {
        return {Processed, Total, !Started};
    }

    void UpdateProgress(ui64 processed)
    {
        Processed += processed;
    }

    void Complete()
    {
        Started = false;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TChannelState
{
    EChannelPermissions Permissions = EChannelPermission::UserWritesAllowed
        | EChannelPermission::SystemWritesAllowed;
    double ApproximateFreeSpaceShare = 0;
    double FreeSpaceScore = 0;

    std::list<NActors::IActorPtr> IORequests;
    size_t IORequestsInFlight = 0;
    size_t IORequestsQueued = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TCompactionScores
{
    float Score = 0;
    ui32 GarbageScore = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TBackpressureFeatureConfig
{
    ui32 InputLimit = 0;
    ui32 InputThreshold = 0;
    double MaxValue = 0;
};

struct TBackpressureFeaturesConfig
{
    TBackpressureFeatureConfig CompactionScoreFeatureConfig;
    TBackpressureFeatureConfig FreshByteCountFeatureConfig;
};

////////////////////////////////////////////////////////////////////////////////

struct TFreeSpaceConfig
{
    double ChannelFreeSpaceThreshold = 0;
    double ChannelMinFreeSpace = 0;
};

////////////////////////////////////////////////////////////////////////////////

class TPartitionState
{
private:
    NProto::TPartitionMeta Meta;
    const ui32 Generation;
    const ICompactionPolicyPtr CompactionPolicy;
    const TBackpressureFeaturesConfig BPConfig;
    const TFreeSpaceConfig FreeSpaceConfig;

public:
    TPartitionState(
            NProto::TPartitionMeta meta,
            ui32 generation,
            ICompactionPolicyPtr compactionPolicy,
            ui32 compactionScoreHistorySize,
            ui32 cleanupScoreHistorySize,
            const TBackpressureFeaturesConfig& bpConfig,
            const TFreeSpaceConfig& freeSpaceConfig,
            ui32 maxIORequestsInFlight,
            ui32 lastCommitId,
            ui32 channelCount,
            ui32 mixedIndexCacheSize,
            bool calculateLogicalUsedBlocks);

private:
    bool LoadStateFinished = false;

public:
    void FinishLoadState()
    {
        LoadStateFinished = true;
    }

    bool IsLoadStateFinished() const
    {
        return LoadStateFinished;
    }

    //
    // Config
    //

private:
    NProto::TPartitionConfig& Config;

public:
    const NProto::TPartitionMeta& GetMeta() const
    {
        return Meta;
    }

    const NProto::TPartitionConfig& GetConfig() const
    {
        return Config;
    }

    const TString& GetBaseDiskId() const
    {
        return Config.GetBaseDiskId();
    }

    const TString& GetBaseDiskCheckpointId() const
    {
        return Config.GetBaseDiskCheckpointId();
    }

    ui32 GetBlockSize() const
    {
        return Config.GetBlockSize();
    }

    ui32 GetMaxBlocksInBlob() const
    {
        return Config.GetMaxBlocksInBlob()
            ? Config.GetMaxBlocksInBlob()
            : MaxBlocksCount;
    }

    ui64 GetBlocksCount() const
    {
        return Config.GetBlocksCount();
    }

    bool CheckBlockRange(const TBlockRange64& range) const;

    //
    // Channels
    //

private:
    TVector<TChannelState> Channels;
    TVector<ui32> FreshChannels;
    ui32 FreshChannelSelector = -1;
    TVector<ui32> MixedChannels;
    bool HaveSeparateMixedChannels = false;
    ui32 MixedChannelSelector = -1;
    TVector<ui32> MergedChannels;
    ui32 MergedChannelSelector = -1;
    double SystemChannelSpaceScoreSum = 0;
    double DataChannelSpaceScoreSum = 0;
    double FreshChannelSpaceScoreSum = 0;
    double BackpressureDiskSpaceScore = 1;
    ui32 ChannelCount = 0;
    ui32 DataChannelCount = 0;
    ui32 FreshChannelCount = 0;
    ui32 AlmostFullChannelCount = 0;

    const ui32 MaxIORequestsInFlight;

public:
    ui32 GetChannelCount() const
    {
        return ChannelCount;
    }

    ui32 GetFreshChannelCount() const
    {
        return FreshChannelCount;
    }

    EChannelDataKind GetChannelDataKind(ui32 channel) const;
    TVector<ui32> GetChannelsByKind(
        std::function<bool(EChannelDataKind)> predicate) const;

    bool UpdatePermissions(ui32 channel, EChannelPermissions permissions);
    bool CheckPermissions(ui32 channel, EChannelPermissions permissions) const;
    bool UpdateChannelFreeSpaceShare(ui32 channel, double share);
    bool CheckChannelFreeSpaceShare(ui32 channel) const;
    bool IsCompactionAllowed() const;
    bool IsWriteAllowed(EChannelPermissions permissions) const;
    TVector<ui32> GetChannelsToReassign() const;
    TBackpressureReport CalculateCurrentBackpressure() const;
    ui32 GetAlmostFullChannelCount() const;

    void EnqueueIORequest(ui32 channel, NActors::IActorPtr requestActor);
    NActors::IActorPtr DequeueIORequest(ui32 channel);
    void CompleteIORequest(ui32 channel);
    ui32 GetIORequestsInFlight() const;
    ui32 GetIORequestsQueued() const;

    TPartialBlobId GenerateBlobId(
        EChannelDataKind kind,
        EChannelPermissions permissions,
        ui64 commitId,
        ui32 blobSize,
        ui32 blobIndex = 0);

    ui32 PickNextChannel(
        EChannelDataKind kind,
        EChannelPermissions permissions);

private:
    void InitChannels();

    TChannelState& GetChannel(ui32 channel);
    const TChannelState* GetChannel(ui32 channel) const;

    bool UpdateChannelFreeSpaceScore(TChannelState& channelState, ui32 channel);

    //
    // Commits
    //

private:
    TCommitQueue CommitQueue;
    ui32 LastCommitId = 0;

public:
    TCommitQueue& GetCommitQueue()
    {
        return CommitQueue;
    }

    ui64 GetLastCommitId() const
    {
        return MakeCommitId(Generation, LastCommitId);
    }

    ui64 GenerateCommitId()
    {
        if (LastCommitId == Max<ui32>()) {
            return InvalidCommitId;
        }
        return MakeCommitId(Generation, ++LastCommitId);
    }

    //
    // Flush
    //

private:
    TOperationState FlushState;
    TRequestBuffer<TWriteBufferRequestData> WriteBuffer;
    ui32 FreshBlocksInFlight = 0;
    ui32 UnflushedFreshBlobCount = 0;
    ui64 UnflushedFreshBlobByteCount = 0;
    ui64 LastFlushToCommitId = 0;
    THashSet<ui64> FlushedCommitIdsInProgress;

public:
    TOperationState& GetFlushState()
    {
        return FlushState;
    }

    TRequestBuffer<TWriteBufferRequestData>& GetWriteBuffer()
    {
        return WriteBuffer;
    }

    ui32 GetFreshBlocksQueued() const
    {
        return WriteBuffer.GetWeight();
    }

    ui32 GetFreshBlocksInFlight() const
    {
        return FreshBlocksInFlight;
    }

    ui32 GetUnflushedFreshBlobCount() const
    {
        return UnflushedFreshBlobCount;
    }

    ui32 GetUnflushedFreshBlobByteCount() const
    {
        return UnflushedFreshBlobByteCount;
    }

    void IncrementUnflushedFreshBlobCount(ui32 value);
    void DecrementUnflushedFreshBlobCount(ui32 value);
    void IncrementUnflushedFreshBlobByteCount(ui64 value);
    void DecrementUnflushedFreshBlobByteCount(ui64 value);

    ui64 GetLastFlushToCommitId() const
    {
        return LastFlushToCommitId;
    }

    void SetLastFlushToCommitId(ui64 commitId)
    {
        LastFlushToCommitId = commitId;
    }

    ui32 IncrementFreshBlocksInFlight(size_t value);
    ui32 DecrementFreshBlocksInFlight(size_t value);

    THashSet<ui64>& GetFlushedCommitIdsInProgress()
    {
        return FlushedCommitIdsInProgress;
    }

    //
    // Fresh blobs
    //

public:
    struct TFreshBlobMeta
    {
        const ui64 CommitId;
        const ui64 BlobSize;

        bool operator<(const TFreshBlobMeta& other) const
        {
            return CommitId < other.CommitId;
        }
    };

private:
    ui64 UntrimmedFreshBlobByteCount = 0;
    TSet<TFreshBlobMeta> UntrimmedFreshBlobs;

public:
    ui64 GetUntrimmedFreshBlobByteCount() const
    {
        return UntrimmedFreshBlobByteCount;
    }

    void AddFreshBlob(TFreshBlobMeta freshBlobMeta);
    void TrimFreshBlobs(ui64 commitId);

    //
    // Fresh Blocks
    //

private:
    ui32 UnflushedFreshBlocksFromChannelCount = 0;
    TBlockIndex Blocks;

    void WriteFreshBlocksImpl(
        TPartitionDatabase& db,
        const TBlockRange32& writeRange,
        ui64 commitId,
        auto getBlockContent)
    {
        TVector<ui64> checkpoints;
        Checkpoints.GetCommitIds(checkpoints);
        SortUnique(checkpoints, TGreater<ui64>());

        TVector<ui64> existingCommitIds;
        TVector<ui64> garbage;

        for (ui32 blockIndex: xrange(writeRange)) {
            ui32 index = blockIndex - writeRange.Start;
            const auto& blockContent = getBlockContent(index);

            Blocks.GetCommitIds(blockIndex, existingCommitIds);

            NCloud::NStorage::FindGarbageVersions(checkpoints, existingCommitIds, garbage);
            for (auto garbageCommitId: garbage) {
                // This block is being flushed; we'll remove it on AddBlobs
                // and we'll release barrier on FlushCompleted
                if (FlushedCommitIdsInProgress.contains(garbageCommitId)) {
                    continue;
                }

                // if block is stored in fresh channel, we'll not remove it,
                // It will be flushed on the next FlushRequest.
                // There will be no more blocks in fresh channel after that
                bool removed = Blocks.RemoveBlock(
                    blockIndex,
                    garbageCommitId,
                    true);  // isStoredInDb

                if (removed) {
                    db.DeleteFreshBlock(blockIndex, garbageCommitId);
                    DecrementUnflushedFreshBlocksFromDbCount(1);
                }
            }

            Blocks.AddBlock(
                blockIndex,
                commitId,
                true,  // isStoredInDb
                blockContent.AsStringBuf());

            db.WriteFreshBlock(blockIndex, commitId, blockContent);

            existingCommitIds.clear();
            garbage.clear();
        }

        IncrementUnflushedFreshBlocksFromDbCount(writeRange.Size());
    }

    void WriteFreshBlocksImpl(
        const TBlockRange32& writeRange,
        ui64 commitId,
        auto getBlockContent)
    {
        TVector<ui64> checkpoints;
        Checkpoints.GetCommitIds(checkpoints);
        SortUnique(checkpoints, TGreater<ui64>());

        TVector<ui64> existingCommitIds;
        TVector<ui64> garbage;

        for (ui32 blockIndex: xrange(writeRange)) {
            ui32 index = blockIndex - writeRange.Start;
            const auto& blockContent = getBlockContent(index);

            Blocks.GetCommitIds(blockIndex, existingCommitIds);

            NCloud::NStorage::FindGarbageVersions(checkpoints, existingCommitIds, garbage);
            for (auto garbageCommitId: garbage) {
                // This block is being flushed; we'll remove it on AddBlobs
                // and we'll release barrier on FlushCompleted
                if (FlushedCommitIdsInProgress.contains(garbageCommitId)) {
                    continue;
                }

                // Do not remove block if it is stored in db
                // to be able to remove it during flush, otherwise
                // we'll leave garbage in FreshBlocksTable
                auto removed = Blocks.RemoveBlock(
                    blockIndex,
                    garbageCommitId,
                    false);  // isStoredInDb

                if (removed) {
                    DecrementUnflushedFreshBlocksFromChannelCount(1);
                    TrimFreshLogBarriers.ReleaseBarrier(garbageCommitId);
                }
            }

            Blocks.AddBlock(
                blockIndex,
                commitId,
                false,  // isStoredInDb
                blockContent.AsStringBuf());

            existingCommitIds.clear();
            garbage.clear();
        }

        IncrementUnflushedFreshBlocksFromChannelCount(writeRange.Size());
    }

public:
    void InitFreshBlocks(const TVector<TOwningFreshBlock>& freshBlocks);

    void FindFreshBlocks(
        IFreshBlocksIndexVisitor& visitor,
        const TBlockRange32& readRange,
        ui64 maxCommitId = Max());

    void WriteFreshBlocks(
        TPartitionDatabase& db,
        const TBlockRange32& writeRange,
        ui64 commitId,
        TSgList sglist);

    void WriteFreshBlocks(
        const TBlockRange32& writeRange,
        ui64 commitId,
        TSgList sglist);

    void ZeroFreshBlocks(
        TPartitionDatabase& db,
        const TBlockRange32& zeroRange,
        ui64 commitId);

    void ZeroFreshBlocks(
        const TBlockRange32& zeroRange,
        ui64 commitId);

    void DeleteFreshBlock(
        TPartitionDatabase& db,
        ui32 blockIndex,
        ui64 commitId);

    void DeleteFreshBlock(
        ui32 blockIndex,
        ui64 commitId);

    ui32 GetUnflushedFreshBlocksCount() const
    {
        return Stats.GetFreshBlocksCount() + UnflushedFreshBlocksFromChannelCount;
    }

    ui32 IncrementUnflushedFreshBlocksFromDbCount(size_t value);
    ui32 DecrementUnflushedFreshBlocksFromDbCount(size_t value);
    ui32 IncrementUnflushedFreshBlocksFromChannelCount(size_t value);
    ui32 DecrementUnflushedFreshBlocksFromChannelCount(size_t value);

    //
    // Mixed blocks
    //

private:
    TProfilingAllocator MixedIndexCacheAllocator;
    TMixedIndexCache MixedIndexCache;

public:
    void WriteMixedBlock(TPartitionDatabase& db, TMixedBlock block);
    void WriteMixedBlocks(
        TPartitionDatabase& db,
        const TPartialBlobId& blobId,
        const TVector<ui32>& blockIndices);

    void DeleteMixedBlock(
        TPartitionDatabase& db,
        ui32 blockIndex,
        ui64 commitId);

    bool FindMixedBlocksForCompaction(
        TPartitionDatabase& db,
        IBlocksIndexVisitor& visitor,
        ui32 rangeIndex);

    void RaiseRangeTemperature(ui32 rangeIndex);

    ui64 GetMixedIndexCacheMemSize() const;

    //
    // Compaction
    //

private:
    TOperationState CompactionState;
    TCompactionMap CompactionMap;
    TRingBuffer<TCompactionScores> CompactionScoreHistory;
    TCompressedBitmap UsedBlocks;
    TCompressedBitmap LogicalUsedBlocks;
    TDuration LastCompactionExecTime;
    TInstant LastCompactionFinishTs;
    TDuration CompactionDelay;

    const bool CalculateLogicalUsedBlocks;

public:
    TOperationState& GetCompactionState()
    {
        return CompactionState;
    }

    TCompactionMap& GetCompactionMap()
    {
        return CompactionMap;
    }

    TRingBuffer<TCompactionScores>& GetCompactionScoreHistory()
    {
        return CompactionScoreHistory;
    }

    void SetLastCompactionExecTime(const TDuration d, const TInstant finishTs)
    {
        LastCompactionExecTime = d;
        LastCompactionFinishTs = finishTs;
    }

    TDuration GetCompactionExecTimeForLastSecond(const TInstant now) const
    {
        // we are interested only in the last compaction's exec time, not the
        // total time spent by all compaction requests that happened in the
        // last second
        return Min(
            LastCompactionExecTime,
            TDuration::Seconds(1) - (now - LastCompactionFinishTs)
        );
    }

    void SetCompactionDelay(const TDuration d)
    {
        CompactionDelay = d;
    }

    TDuration GetCompactionDelay() const
    {
        return CompactionDelay;
    }

    ui32 GetLegacyCompactionScore() const
    {
        return CompactionMap.GetTop().Stat.BlobCount;
    }

    ui32 GetCompactionGarbageScore() const
    {
        return CompactionMap.GetTopByGarbageBlockCount().Stat.GarbageBlockCount();
    }

    float GetCompactionScore() const
    {
        return CompactionMap.GetTop().Stat.Score;
    }

    TCompressedBitmap& GetUsedBlocks()
    {
        return UsedBlocks;
    }

    TCompressedBitmap& GetLogicalUsedBlocks()
    {
        return LogicalUsedBlocks;
    }

    void SetUsedBlocks(TPartitionDatabase& db, const TBlockRange32& range, ui32 skipCount);
    void SetUsedBlocks(TPartitionDatabase& db, const TVector<ui32>& blocks);
    void UnsetUsedBlocks(TPartitionDatabase& db, const TBlockRange32& range);
    void UnsetUsedBlocks(TPartitionDatabase& db, const TVector<ui32>& blocks);

private:
    void WriteUsedBlocksToDB(TPartitionDatabase& db, ui32 begin, ui32 end);

    //
    // Forced Compaction
    //

private:
    TForcedCompactionState ForcedCompactionState;

public:
    bool IsForcedCompactionRunning() const
    {
        return ForcedCompactionState.IsRunning;
    }

    void StartForcedCompaction(const TString& operationId, ui32 blocksCount)
    {
        ForcedCompactionState.IsRunning = true;
        ForcedCompactionState.Progress = 0;
        ForcedCompactionState.RangesCount = blocksCount;
        ForcedCompactionState.OperationId = operationId;
    }

    void OnNewCompactionRange()
    {
        ++ForcedCompactionState.Progress;
    }

    void ResetForcedCompaction()
    {
        ForcedCompactionState.IsRunning = false;
        ForcedCompactionState.Progress = 0;
        ForcedCompactionState.RangesCount = 0;
        ForcedCompactionState.OperationId.clear();
    }

    const TForcedCompactionState& GetForcedCompactionState() const
    {
        return ForcedCompactionState;
    }

    //
    // Metadata Rebuild
    //

private:
    TMetadataRebuildState RebuildState;

public:
    bool IsMetadataRebuildStarted() const
    {
        return RebuildState.IsStarted();
    }

    EMetadataRebuildType GetMetadataRebuildType() const
    {
        return RebuildState.GetType();
    }

    void StartRebuildUsedBlocks()
    {
        RebuildState.StartRebuildUsedBlocks(
            GetBlocksCount());
    }

    void StartRebuildBlockCount()
    {
        RebuildState.StartRebuildBlockCount(
            GetMixedBlobsCount(),
            GetMergedBlobsCount());
    }

    TMedatadataRebuildProgress GetMetadataRebuildProgress() const
    {
        return RebuildState.GetProgress();
    }

    void UpdateRebuildMetadataProgress(ui64 processed)
    {
        RebuildState.UpdateProgress(processed);
    }

    void CompleteMetadataRebuild()
    {
        RebuildState.Complete();
    }

    void UpdateBlocksCountersAfterMetadataRebuild(ui64 mixed, ui64 merged)
    {
        Stats.SetMixedBlocksCount(mixed);
        Stats.SetMergedBlocksCount(merged);
    }

    //
    // Cleanup
    //

private:
    TOperationState CleanupState;
    TCheckpointStore Checkpoints;
    TCheckpointsInFlight CheckpointsInFlight;
    TCleanupQueue CleanupQueue;
    TRingBuffer<ui32> CleanupScoreHistory;

    mutable ui32 BlobCountToCleanup = 0;
    mutable ui64 BlobCountToCleanupCommitId = 0;

    TDuration LastCleanupExecTime;
    TInstant LastCleanupFinishTs;
    TDuration CleanupDelay;

public:
    TOperationState& GetCleanupState()
    {
        return CleanupState;
    }

    TCleanupQueue& GetCleanupQueue()
    {
        return CleanupQueue;
    }

    ui32 GetBlobCountToCleanup(ui64 commitId, ui32 maxBlobs) const
    {
        if (commitId < BlobCountToCleanupCommitId
                || BlobCountToCleanup < maxBlobs)
        {
            BlobCountToCleanup = CleanupQueue.GetCount(commitId);
            BlobCountToCleanupCommitId = commitId;
        }

        return BlobCountToCleanup;
    }

    void RemoveCleanupQueueItem(const TCleanupQueueItem& item)
    {
        bool removed = CleanupQueue.Remove(item);
        Y_VERIFY(removed);

        // BlobCountToCleanup is not perfectly synchronized with CleanupQueue:
        // it can actually be smaller
        if (BlobCountToCleanup) {
            --BlobCountToCleanup;
        }
    }

    TRingBuffer<ui32>& GetCleanupScoreHistory()
    {
        return CleanupScoreHistory;
    }

    void SetLastCleanupExecTime(const TDuration d, const TInstant finishTs)
    {
        LastCleanupExecTime = d;
        LastCleanupFinishTs = finishTs;
    }

    TDuration GetCleanupExecTimeForLastSecond(const TInstant now) const
    {
        // TODO: unify this code and compaction delay-related code
        // we are interested only in the last cleanup's exec time, not the
        // total time spent by all cleanup requests that happened in the
        // last second
        return Min(
            LastCleanupExecTime,
            TDuration::Seconds(1) - (now - LastCleanupFinishTs)
        );
    }

    void SetCleanupDelay(const TDuration d)
    {
        CleanupDelay = d;
    }

    TDuration GetCleanupDelay() const
    {
        return CleanupDelay;
    }

    TCheckpointStore& GetCheckpoints()
    {
        return Checkpoints;
    }

    TCheckpointsInFlight& GetCheckpointsInFlight()
    {
        return CheckpointsInFlight;
    }

    ui64 GetCleanupCommitId() const;

    ui64 CalculateCheckpointBytes() const;

    //
    // Garbage collection
    //

private:
    TOperationState CollectGarbageState;
    TGarbageQueue GarbageQueue;
    ui32 LastCollectCounter = 0;
    TDuration CollectTimeout;
    bool StartupGcExecuted = false;

public:
    TOperationState& GetCollectGarbageState()
    {
        return CollectGarbageState;
    }

    TGarbageQueue& GetGarbageQueue()
    {
        return GarbageQueue;
    }

    TDuration GetCollectTimeout()
    {
        return CollectTimeout;
    }

    void RegisterCollectError()
    {
        CollectTimeout = Min(
            TDuration::Seconds(5),
            Max(TDuration::MilliSeconds(100), CollectTimeout * 2)
        );
    }

    void RegisterCollectSuccess()
    {
        CollectTimeout = {};
    }

    ui64 GetLastCollectCommitId() const
    {
        return Meta.GetLastCollectCommitId();
    }

    void SetLastCollectCommitId(ui64 commitId)
    {
        Meta.SetLastCollectCommitId(commitId);
    }

    ui32 NextCollectCounter()
    {
        if (LastCollectCounter == InvalidCollectCounter) {
            return InvalidCollectCounter;
        }
        return ++LastCollectCounter;
    }

    ui64 GetCollectCommitId() const;

    bool GetStartupGcExecuted() const
    {
        return StartupGcExecuted;
    }

    void SetStartupGcExecuted()
    {
        StartupGcExecuted = true;
    }

    bool CollectGarbageHardRequested = false;

    //
    // TrimFreshLog
    //

private:
    TBarriers TrimFreshLogBarriers;
    TOperationState TrimFreshLogState;
    ui64 LastTrimFreshLogToCommitId = 0;
    TDuration TrimFreshLogTimeout;

public:
    TBarriers& GetTrimFreshLogBarriers()
    {
        return TrimFreshLogBarriers;
    }

    TOperationState& GetTrimFreshLogState()
    {
        return TrimFreshLogState;
    }

    TDuration GetTrimFreshLogTimeout()
    {
        return TrimFreshLogTimeout;
    }

    void RegisterTrimFreshLogError()
    {
        TrimFreshLogTimeout = Min(
            TDuration::Seconds(5),
            Max(TDuration::MilliSeconds(100), TrimFreshLogTimeout * 2)
        );
    }

    void RegisterTrimFreshLogSuccess()
    {
        TrimFreshLogTimeout = {};
    }

    ui64 GetLastTrimFreshLogToCommitId() const
    {
        return LastTrimFreshLogToCommitId;
    }

    void SetLastTrimFreshLogToCommitId(ui64 commitId)
    {
        LastTrimFreshLogToCommitId = commitId;
    }

    //
    // ReadBlob
    //

private:
    ui32 ReadBlobErrorCount = 0;

public:
    ui32 IncrementReadBlobErrorCount()
    {
        return ++ReadBlobErrorCount;
    }

    //
    // Stats
    //

private:
    NProto::TPartitionStats& Stats;
    const int MaxSufferScore = 5;
    int ReadSufferScore = 0;
    int WriteSufferScore = 0;

public:
    const NProto::TPartitionStats& GetStats() const
    {
        return Stats;
    }

    NProto::TPartitionStats& AccessStats()
    {
        return Stats;
    }

    void RegisterReadSuffer(bool suffer)
    {
        if (suffer) {
            ReadSufferScore = MaxSufferScore;
        } else if (ReadSufferScore > 0) {
            --ReadSufferScore;
        }
    }

    bool IsReadSuffering() const
    {
        return ReadSufferScore > 0;
    }

    void RegisterWriteSuffer(bool suffer)
    {
        if (suffer) {
            WriteSufferScore = MaxSufferScore;
        } else if (WriteSufferScore > 0) {
            --WriteSufferScore;
        }
    }

    bool IsWriteSuffering() const
    {
        return WriteSufferScore > 0;
    }

#define BLOCKSTORE_PARTITION_DECLARE_COUNTER(name)                             \
    ui64 Get##name() const                                                     \
    {                                                                          \
        return Stats.Get##name();                                              \
    }                                                                          \
                                                                               \
    ui64 Increment##name(size_t value);                                        \
    ui64 Decrement##name(size_t value);                                        \
// BLOCKSTORE_PARTITION_DECLARE_COUNTER

    BLOCKSTORE_PARTITION_PROTO_COUNTERS(BLOCKSTORE_PARTITION_DECLARE_COUNTER)

#undef BLOCKSTORE_PARTITION_DECLARE_COUNTER

    template <typename T>
    void UpdateStats(T&& update)
    {
        update(Stats);
    }

    void DumpHtml(IOutputStream& out) const;
    NJson::TJsonValue AsJson() const;
};

}   // namespace NCloud::NBlockStore::NStorage::NPartition
