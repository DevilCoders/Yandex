#include "volume_actor.h"

#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <cloud/storage/core/libs/common/media.h>

#include <util/system/hostname.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_CACHED_COUNTERS(xxx, ...)                                   \
    xxx(MixedBytesCount,                                           __VA_ARGS__)\
    xxx(MergedBytesCount,                                          __VA_ARGS__)\
    xxx(FreshBytesCount,                                           __VA_ARGS__)\
    xxx(UsedBytesCount,                                            __VA_ARGS__)\
    xxx(LogicalUsedBytesCount,                                     __VA_ARGS__)\
    xxx(BytesCount,                                                __VA_ARGS__)\
    xxx(CheckpointBytes,                                           __VA_ARGS__)\
    xxx(CompactionScore,                                           __VA_ARGS__)\
    xxx(CompactionGarbageScore,                                    __VA_ARGS__)\
    xxx(CleanupQueueBytes,                                         __VA_ARGS__)\
    xxx(GarbageQueueBytes,                                         __VA_ARGS__)\
    xxx(ChannelHistorySize,                                        __VA_ARGS__)\
// BLOCKSTORE_CACHED_COUNTERS

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::CopyCachedStatsToPartCounters(
    const NProto::TCachedPartStats& src,
    TPartitionDiskCounters& dst)
{
#define POPULATE_COUNTERS(name, ...)                                           \
    dst.Simple.name.Set(src.Get##name());                                     \
// POPULATE_COUNTERS

    BLOCKSTORE_CACHED_COUNTERS(POPULATE_COUNTERS)
#undef POPULATE_COUNTERS
}

void TVolumeActor::CopyPartCountersToCachedStats(
    const TPartitionDiskCounters& src,
    NProto::TCachedPartStats& dst)
{
#define CACHE_COUNTERS(name, ...)                                             \
    dst.Set##name(src.Simple.name.Value);                                     \
// CACHE_COUNTERS

    BLOCKSTORE_CACHED_COUNTERS(CACHE_COUNTERS)
#undef CACHE_COUNTERS
}

void TVolumeActor::UpdateCachedStats(
    const TPartitionDiskCounters& src,
    TPartitionDiskCounters& dst)
{
#define UPDATE_COUNTERS(name, ...)                                            \
    dst.Simple.name.Value = src.Simple.name.Value;                            \
// UPDATE_COUNTERS

    BLOCKSTORE_CACHED_COUNTERS(UPDATE_COUNTERS)
#undef UPDATE_COUNTERS
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::UpdateActorStats(const TActorContext& ctx)
{
    if (Counters) {
        auto& actorQueue = Counters->Percentile()
            [TVolumeCounters::PERCENTILE_COUNTER_Actor_ActorQueue];
        auto& mailboxQueue = Counters->Percentile()
            [TVolumeCounters::PERCENTILE_COUNTER_Actor_MailboxQueue];

        auto actorQueues = ctx.CountMailboxEvents(1001);
        IncrementFor(actorQueue, actorQueues.first);
        IncrementFor(mailboxQueue, actorQueues.second);
    }
}

void TVolumeActor::HandlePartStatsSaved(
    const TEvVolumePrivate::TEvPartStatsSaved::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    Y_UNUSED(ctx);
}

void TVolumeActor::HandleNonreplicatedPartCounters(
    const TEvVolume::TEvNonreplicatedPartitionCounters::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId)
    );

    if (State->GetNonreplicatedPartitionActor() != ev->Sender) {
        LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
            "Partition %s for disk %s counters not found",
            ToString(ev->Sender).c_str(),
            State->GetDiskId().Quote().c_str());
        return;
    }

    auto& info = State->GetPartitionStatInfos().front();
    if (!info.LastCounters) {
        info.LastCounters = CreatePartitionDiskCounters(CountersPolicy);
    }

    info.LastCounters->Add(*msg->DiskCounters);

    UpdateCachedStats(*msg->DiskCounters, info.CachedCounters);

    TVolumeDatabase::TPartStats partStats;

    auto kind = State->GetConfig().GetStorageMediaKind();
    Y_VERIFY_DEBUG(IsDiskRegistryMediaKind(kind));

    CopyPartCountersToCachedStats(*msg->DiskCounters, partStats.Stats);

    ExecuteTx<TSavePartStats>(
        ctx,
        std::move(requestInfo),
        std::move(partStats),
        false
    );
}

void TVolumeActor::HandlePartCounters(
    const TEvStatsService::TEvVolumePartCounters::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId)
    );

    ui32 index = 0;
    if (!State->FindPartitionStatInfoByOwner(ev->Sender, index)) {
        LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
            "Partition %s for disk %s counters not found",
            ToString(ev->Sender).c_str(),
            State->GetDiskId().Quote().c_str());
        return;
    }

    auto& info = State->GetPartitionStatInfos()[index];
    if (!info.LastCounters) {
        info.LastCounters = CreatePartitionDiskCounters(CountersPolicy);
        info.LastMetrics = std::move(msg->BlobLoadMetrics);
    }

    info.LastSystemCpu += msg->VolumeSystemCpu;
    info.LastUserCpu += msg->VolumeUserCpu;

    info.LastCounters->Add(*msg->DiskCounters);

    UpdateCachedStats(*msg->DiskCounters, info.CachedCounters);

    TVolumeDatabase::TPartStats partStats;

    auto kind = State->GetConfig().GetStorageMediaKind();
    Y_VERIFY_DEBUG(!IsDiskRegistryMediaKind(kind));
    partStats.Id = State->GetPartitions()[index].TabletId;

    CopyPartCountersToCachedStats(*msg->DiskCounters, partStats.Stats);

    ExecuteTx<TSavePartStats>(
        ctx,
        std::move(requestInfo),
        std::move(partStats),
        true
    );
}

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::PrepareSavePartStats(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TSavePartStats& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TVolumeActor::ExecuteSavePartStats(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TSavePartStats& args)
{
    Y_UNUSED(ctx);

    TVolumeDatabase db(tx.DB);
    if (args.IsReplicatedVolume) {
        db.WritePartStats(args.PartStats.Id, args.PartStats.Stats);
    } else {
        db.WriteNonReplPartStats(args.PartStats.Id, args.PartStats.Stats);
    }
}

void TVolumeActor::CompleteSavePartStats(
    const TActorContext& ctx,
    TTxVolume::TSavePartStats& args)
{
    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Part %lu stats saved",
        TabletID(),
        args.PartStats.Id);

    NCloud::Send(
        ctx,
        SelfId(),
        std::make_unique<TEvVolumePrivate::TEvPartStatsSaved>()
    );
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::SendPartStatsToService(const TActorContext& ctx)
{
    auto stats = CreatePartitionDiskCounters(CountersPolicy);
    ui64 systemCpu = 0;
    ui64 userCpu = 0;
    // XXX - we need to "manually" calculate total channel history
    // size here because this counter is different from all other
    // partition counters. In volume it should be aggregated as
    // simple counter, but in service it should be aggregated as max counter.
    // Fix this if there are several such counters.
    ui64 channelsHistorySize = 0;

    NBlobMetrics::TBlobLoadMetrics offsetPartitionMetrics;

    for (auto& info : State->GetPartitionStatInfos())
    {
        if (!info.LastCounters) {
            stats->AggregateWith(info.CachedCounters);
            channelsHistorySize +=
                info.CachedCounters.Simple.ChannelHistorySize.Value;
        } else {
            stats->AggregateWith(*info.LastCounters);
            channelsHistorySize +=
                info.LastCounters->Simple.ChannelHistorySize.Value;
            systemCpu += info.LastSystemCpu;
            userCpu += info.LastUserCpu;
            offsetPartitionMetrics += info.LastMetrics;
        }

        info.LastCounters = nullptr;
        info.LastSystemCpu = 0;
        info.LastUserCpu = 0;
    }

    stats->Simple.ChannelHistorySize.Set(channelsHistorySize);

    NBlobMetrics::TBlobLoadMetrics blobLoadMetrics = NBlobMetrics::MakeBlobLoadMetrics(
         State->GetMeta().GetVolumeConfig().GetVolumeExplicitChannelProfiles(),
        *Executor()->GetResourceMetrics());
    NBlobMetrics::TBlobLoadMetrics offsetLoadMetrics = NBlobMetrics::TakeDelta(
        PrevMetrics, blobLoadMetrics);
    offsetLoadMetrics += offsetPartitionMetrics;

    auto request = std::make_unique<TEvStatsService::TEvVolumePartCounters>(
        MakeIntrusive<TCallContext>(),
        State->GetConfig().GetDiskId(),
        std::move(stats),
        systemCpu,
        userCpu,
        State->GetActiveCheckpoints().Size(),
        std::move(offsetLoadMetrics));

    PrevMetrics = std::move(blobLoadMetrics);

    NCloud::Send(ctx, MakeStorageStatsServiceId(), std::move(request));
}

void TVolumeActor::SendSelfStatsToService(const TActorContext& ctx)
{
    if (!VolumeSelfCounters) {
        return;
    }

    const auto& pp = State->GetConfig().GetPerformanceProfile();
    auto& simple = VolumeSelfCounters->Simple;
    simple.MaxReadBandwidth.Set(pp.GetMaxReadBandwidth());
    simple.MaxWriteBandwidth.Set(pp.GetMaxWriteBandwidth());
    simple.MaxReadIops.Set(pp.GetMaxReadIops());
    simple.MaxWriteIops.Set(pp.GetMaxWriteIops());

    const auto& tp = State->GetThrottlingPolicy();
    simple.RealMaxWriteBandwidth.Set(
        tp.GetWriteCostMultiplier()
        ? pp.GetMaxWriteBandwidth() / tp.GetWriteCostMultiplier() : 0);
    simple.PostponedQueueWeight.Set(tp.CalculatePostponedWeight());

    const auto& bp = tp.GetCurrentBackpressure();
    simple.BPFreshIndexScore.Set(100 * bp.FreshIndexScore);
    simple.BPCompactionScore.Set(100 * bp.CompactionScore);
    simple.BPDiskSpaceScore.Set(100 * bp.DiskSpaceScore);

    simple.VBytesCount.Set(GetBlocksCount() * State->GetBlockSize());
    simple.PartitionCount.Set(State->GetPartitions().size());

    // XXX
    if (State->GetMeta().GetMigrations().size()) {
        simple.MigrationStarted.Set(true);
        simple.MigrationProgress.Set(
            // 100 * State->GetMigratedBlockCount() / GetBlocksCount()
            100 * State->GetMeta().GetMigrationIndex() / GetBlocksCount()
        );
    }

    SendVolumeSelfCounters(ctx);
    VolumeSelfCounters = CreateVolumeSelfCounters(CountersPolicy);
}

void TVolumeActor::HandleGetVolumeLoadInfo(
    const TEvVolume::TEvGetVolumeLoadInfoRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto response = std::make_unique<TEvVolume::TEvGetVolumeLoadInfoResponse>();

    auto& stats = *response->Record.MutableStats();
    stats.SetDiskId(State->GetDiskId());
    stats.SetCloudId(State->GetMeta().GetVolumeConfig().GetCloudId());

    for (auto& info: State->GetPartitionStatInfos()) {
        if (info.LastCounters) {
            stats.SetSystemCpu(stats.GetSystemCpu() + info.LastSystemCpu);
            stats.SetUserCpu(stats.GetUserCpu() + info.LastUserCpu);
            // TODO: report real number of threads
            stats.SetNumSystemThreads(0);
            stats.SetNumUserThreads(0);
        }
    }

    stats.SetHost(FQDNHostName());

    NCloud::Reply(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
