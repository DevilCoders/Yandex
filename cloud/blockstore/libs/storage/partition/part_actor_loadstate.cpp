#include "part_actor.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

ui64 GetLastCollectCommitId(const TMaybe<NProto::TPartitionMeta>& meta)
{
    return meta ? meta->GetLastCollectCommitId() : 0;
}

ui32 GetMaxIORequestsInFlight(
    const TStorageConfig& config,
    const NProto::TPartitionConfig& partitionConfig)
{
    if (GetThrottlingEnabled(config, partitionConfig)) {
        return Max<ui32>();
    }

    switch (partitionConfig.GetStorageMediaKind()) {
        case NCloud::NProto::EStorageMediaKind::STORAGE_MEDIA_SSD:
            return config.GetMaxIORequestsInFlightSSD();
        default:
            return config.GetMaxIORequestsInFlight();
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::SendGetUsedBlocksFromBaseDisk(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvVolume::TEvGetUsedBlocksRequest>();

    request->Record.SetDiskId(State->GetBaseDiskId());

    TAutoPtr<IEventHandle> event = new IEventHandle(
        MakeVolumeProxyServiceId(),
        SelfId(),
        request.release());

    ctx.Send(event);
}

bool TPartitionActor::PrepareLoadState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TLoadState& args)
{
    LOG_INFO(ctx, TBlockStoreComponents::PARTITION,
        "[%lu] Reading state from local db",
        TabletID());

    // TRequestScope timer(*args.RequestInfo);
    TPartitionDatabase db(tx.DB);

    std::initializer_list<bool> results = {
        db.ReadMeta(args.Meta),
        db.ReadFreshBlocks(args.FreshBlocks),
        db.ReadCompactionMap(args.CompactionMap),
        db.ReadUsedBlocks(args.UsedBlocks),
        db.ReadLogicalUsedBlocks(args.LogicalUsedBlocks, args.ReadLogicalUsedBlocks),
        db.ReadCheckpoints(args.Checkpoints, args.CheckpointId2CommitId),
        db.ReadCleanupQueue(args.CleanupQueue),
        db.ReadGarbageBlobs(args.GarbageBlobs),
    };

    bool ready = std::accumulate(
        results.begin(),
        results.end(),
        true,
        std::logical_and<>()
    );

    if (ready) {
        ready &= db.ReadNewBlobs(args.NewBlobs, GetLastCollectCommitId(args.Meta));
    }

    return ready;
}

void TPartitionActor::ExecuteLoadState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TLoadState& args)
{
    LOG_INFO(ctx, TBlockStoreComponents::PARTITION,
        "[%lu] State data loaded",
        TabletID());

    // TRequestScope timer(*args.RequestInfo);
    TPartitionDatabase db(tx.DB);

    if (!args.Meta) {
        // initialize with empty meta
        args.Meta = NProto::TPartitionMeta();
    }

    // override config
    args.Meta->MutableConfig()->CopyFrom(PartitionConfig);

    db.WriteMeta(*args.Meta);
}

void TPartitionActor::CompleteLoadState(
    const TActorContext& ctx,
    TTxPartition::TLoadState& args)
{
    const auto& partitionConfig = args.Meta->GetConfig();

    // initialize state
    TBackpressureFeaturesConfig bpConfig {
        {
            Config->GetCompactionScoreLimitForBackpressure(),
            Config->GetCompactionScoreThresholdForBackpressure(),
            static_cast<double>(Config->GetCompactionScoreFeatureMaxValue()),
        },
        {
            Config->GetFreshByteCountLimitForBackpressure(),
            Config->GetFreshByteCountThresholdForBackpressure(),
            static_cast<double>(Config->GetFreshByteCountFeatureMaxValue()),
        },
    };

    TFreeSpaceConfig fsConfig {
        Config->GetChannelFreeSpaceThreshold() / 100.,
        Config->GetChannelMinFreeSpace() / 100.,
    };

    ui32 tabletChannelCount = Info()->Channels.size();
    ui32 configChannelCount = partitionConfig.ExplicitChannelProfilesSize();

    if (tabletChannelCount < configChannelCount) {
        // either a race or a bug (if this situation occurs again after tablet restart)
        // example: https://st.yandex-team.ru/CLOUDINC-2027
        ReportInvalidTabletConfig();

        LOG_ERROR_S(ctx, TBlockStoreComponents::PARTITION,
            "[" << TabletID() << "]"
            << "tablet info differs from config: "
            << "tabletChannelCount < configChannelCount ("
            << tabletChannelCount << " < " << configChannelCount << ")");
    } else if (tabletChannelCount > configChannelCount) {
        // legacy channel configuration
         LOG_WARN_S(ctx, TBlockStoreComponents::PARTITION,
            "[" << TabletID() << "]"
            << "tablet info differs from config: "
            << "tabletChannelCount > configChannelCount ("
            << tabletChannelCount << " > " << configChannelCount << ")");
    }

    const ui32 mixedIndexCacheSize = [&] {
        if (!Config->GetMixedIndexCacheV1Enabled() &&
            !Config->IsMixedIndexCacheV1FeatureEnabled(
                partitionConfig.GetCloudId(),
                partitionConfig.GetFolderId()))
        {
            // disabled by default & not enabled for cloud
            return 0u;
        }

        const auto kind = partitionConfig.GetStorageMediaKind();
        if (kind == NCloud::NProto::EStorageMediaKind::STORAGE_MEDIA_SSD) {
            return Config->GetMixedIndexCacheV1SizeSSD();
        }

        return 0u;
    }();

    State = std::make_unique<TPartitionState>(
        *args.Meta,
        Executor()->Generation(),
        BuildCompactionPolicy(partitionConfig, *Config, SiblingCount),
        Config->GetCompactionScoreHistorySize(),
        Config->GetCleanupScoreHistorySize(),
        bpConfig,
        fsConfig,
        GetMaxIORequestsInFlight(*Config, PartitionConfig),
        0,  // lastCommitId
        Min(tabletChannelCount, configChannelCount),  // channelCount
        mixedIndexCacheSize,
        Config->GetLogicalUsedBlocksCalculationEnabled());

    State->InitFreshBlocks(args.FreshBlocks);
    State->GetUsedBlocks() = std::move(args.UsedBlocks);
    State->AccessStats().SetUsedBlocksCount(State->GetUsedBlocks().Count());
    State->GetCompactionMap().Update(
        args.CompactionMap,
        &State->GetUsedBlocks()
    );
    State->GetCheckpoints().Add(args.Checkpoints);
    State->GetCheckpoints().SetCheckpointMappings(args.CheckpointId2CommitId);
    State->GetCleanupQueue().Add(args.CleanupQueue);
    Y_VERIFY(State->GetGarbageQueue().AddNewBlobs(args.NewBlobs));
    Y_VERIFY(State->GetGarbageQueue().AddGarbageBlobs(args.GarbageBlobs));

    if (Config->GetLogicalUsedBlocksCalculationEnabled()) {
        if (State->GetBaseDiskId()) {
            if (args.ReadLogicalUsedBlocks) {
                State->GetLogicalUsedBlocks() = std::move(args.LogicalUsedBlocks);
                State->AccessStats().SetLogicalUsedBlocksCount(
                    State->GetLogicalUsedBlocks().Count()
                );
            } else {
                State->GetLogicalUsedBlocks().Update(State->GetUsedBlocks(), 0);
                State->AccessStats().SetLogicalUsedBlocksCount(
                    State->AccessStats().GetUsedBlocksCount()
                );

                SendGetUsedBlocksFromBaseDisk(ctx);
                return;
            }
        } else {
            State->AccessStats().SetLogicalUsedBlocksCount(
                State->AccessStats().GetUsedBlocksCount()
            );
        }
    }

    FinalizeLoadState(ctx);
}

void TPartitionActor::FinalizeLoadState(const TActorContext& ctx)
{
    auto totalBlocksCount = State->GetMixedBlocksCount() + State->GetMergedBlocksCount();
    UpdateStorageStat(totalBlocksCount * State->GetBlockSize());
    UpdateExecutorStats(ctx);

    LoadFreshBlobs(ctx);
}

void TPartitionActor::HandleGetUsedBlocksResponse(
    const TEvVolume::TEvGetUsedBlocksResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    if (FAILED(msg->GetStatus())) {
        LOG_ERROR_S(ctx, TBlockStoreComponents::PARTITION,
            "[" << TabletID() << "]"
            << " LoadState failed: GetUsedBlocks from base disk failed: " << msg->GetStatus()
            << " reason: " << msg->GetError().GetMessage().Quote());

        Suicide(ctx);
        return;
    } else {
        LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
            "[%lu] LoadState completed",
            TabletID());
    }

    for (const auto& block: msg->Record.GetUsedBlocks()) {
        State->GetLogicalUsedBlocks().Merge(
            TCompressedBitmap::TSerializedChunk{
                block.GetChunkIdx(),
                block.GetData()
            });
    }

    State->AccessStats().SetLogicalUsedBlocksCount(
        State->GetLogicalUsedBlocks().Count()
    );

    ExecuteTx<TUpdateLogicalUsedBlocks>(ctx, 0);
}

bool TPartitionActor::PrepareUpdateLogicalUsedBlocks(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TUpdateLogicalUsedBlocks& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TPartitionActor::ExecuteUpdateLogicalUsedBlocks(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TUpdateLogicalUsedBlocks& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(args);

    TPartitionDatabase db(tx.DB);

    args.UpdatedToIdx = Min(
        args.UpdateFromIdx + Config->GetLogicalUsedBlocksUpdateBlockCount(),
        State->GetLogicalUsedBlocks().Capacity());

    auto serializer = State->GetLogicalUsedBlocks().RangeSerializer(
        args.UpdateFromIdx, args.UpdatedToIdx
    );
    TCompressedBitmap::TSerializedChunk sc;
    while (serializer.Next(&sc)) {
        if (!TCompressedBitmap::IsZeroChunk(sc)) {
            db.WriteLogicalUsedBlocks(sc);
        }
    }
}

void TPartitionActor::CompleteUpdateLogicalUsedBlocks(
    const TActorContext& ctx,
    TTxPartition::TUpdateLogicalUsedBlocks& args)
{
    if (args.UpdatedToIdx == State->GetLogicalUsedBlocks().Capacity()) {
        FinalizeLoadState(ctx);
    } else {
        ExecuteTx<TUpdateLogicalUsedBlocks>(ctx, args.UpdatedToIdx);
    }
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
