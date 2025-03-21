#include "volume_actor.h"

#include "volume_database.h"

#include <cloud/blockstore/libs/storage/core/config.h>

#include <cloud/storage/core/libs/common/media.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::PrepareLoadState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TLoadState& args)
{
    Y_UNUSED(ctx);

    TVolumeDatabase db(tx.DB);
    auto now = ctx.Now();

    std::initializer_list<bool> results = {
        db.ReadMeta(args.Meta),
        db.ReadClients(args.Clients),
        db.ReadOutdatedHistory(args.OutdatedHistory, args.OldestLogEntry),
        db.ReadHistory(
            args.History,
            now,
            args.OldestLogEntry,
            Config->GetVolumeHistoryCacheSize()),
        db.ReadPartStats(args.PartStats),
        db.ReadNonReplPartStats(args.PartStats),
        db.CollectCheckpointsToDelete(
            Config->GetDeletedCheckpointHistoryLifetime(),
            now,
            args.DeletedCheckpoints),
        db.ReadCheckpointRequests(
            args.DeletedCheckpoints,
            args.CheckpointRequests,
            args.OutdatedCheckpointRequestIds),
        db.ReadThrottlerState(args.ThrottlerStateInfo),
    };

    bool ready = std::accumulate(
        results.begin(),
        results.end(),
        true,
        std::logical_and<>()
    );

    if (ready && args.Meta) {
        args.UsedBlocks.ConstructInPlace(ComputeBlockCount(*args.Meta));
        ready &= db.ReadUsedBlocks(*args.UsedBlocks);
    }

    return ready;
}

void TVolumeActor::ExecuteLoadState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TLoadState& args)
{
    TVolumeDatabase db(tx.DB);

    // Consider all clients loaded from persistent state disconnected
    // with disconnect time being the time of state loading. Set disconnect
    // timestamp only if it was not set before.
    // If these clients are still alive, they'd re-establish pipe connections
    // with the volume shortly and send AddClientRequest.
    auto now = ctx.Now();
    bool anyChanged = false;
    for (auto& client : args.Clients) {
        if (!client.second.GetVolumeClientInfo().GetDisconnectTimestamp()) {
            client.second.SetDisconnectTimestamp(now);
            anyChanged = true;
        }
    }

    if (anyChanged) {
        db.WriteClients(args.Clients);
    }

    for (const auto& o: args.OutdatedHistory) {
        db.DeleteHistoryEntry(o);
    }

    for (const auto& o: args.OutdatedCheckpointRequestIds) {
        db.DeleteCheckpointEntry(o);
    }

    LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] State data loaded",
        TabletID());
}

void TVolumeActor::CompleteLoadState(
    const TActorContext& ctx,
    TTxVolume::TLoadState& args)
{
    if (args.Meta.Defined()) {
        TThrottlerConfig throttlerConfig(
            Config->GetMaxThrottlerDelay(),
            Config->GetMaxWriteCostMultiplier(),
            Config->GetDefaultPostponedRequestWeight(),
            args.ThrottlerStateInfo.Defined()
                ? TDuration::MilliSeconds(args.ThrottlerStateInfo->Budget)
                : CalculateBoostTime(
                    args.Meta->GetConfig().GetPerformanceProfile())
        );

        State.reset(new TVolumeState(
            Config,
            std::move(*args.Meta),
            throttlerConfig,
            std::move(args.Clients),
            std::move(args.History),
            std::move(args.CheckpointRequests)));

        for (const auto& partStats: args.PartStats) {
            const auto& stats = partStats.Stats;

            // info doesn't have to be always present
            // see https://st.yandex-team.ru/NBS-1668#603e955e319cc33b747904fb
            if (auto* info = State->GetPartitionStatInfoById(partStats.Id)) {
                CopyCachedStatsToPartCounters(stats, info->CachedCounters);
            }
        }

        Y_VERIFY(CurrentState == STATE_INIT);
        BecomeAux(ctx, STATE_WORK);

        LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
            "[%lu] State initialization finished",
            TabletID());

        RegisterVolume(ctx);

        const bool isNonrepl =
            IsDiskRegistryMediaKind(State->GetConfig().GetStorageMediaKind());
        if (isNonrepl) {
            CountersPolicy = EPublishingPolicy::NonRepl;
        } else {
            CountersPolicy = EPublishingPolicy::Repl;
        }

        if (isNonrepl || PendingRequests.size()) {
            StartPartitions(ctx);
        }

        for (const auto& checkpointRequest: State->GetCheckpointRequests()) {
            if (checkpointRequest.State != ECheckpointRequestState::Completed) {
                CheckpointRequestQueue.emplace_back(
                    CreateRequestInfo(
                        SelfId(),
                        0,  // cookie
                        MakeIntrusive<TCallContext>()
                    ),
                    checkpointRequest.RequestId,
                    checkpointRequest.CheckpointId,
                    checkpointRequest.ReqType,
                    false,
                    GetCycleCount()
                );
            }
        }
    }

    if (args.UsedBlocks) {
        State->AccessUsedBlocks() = std::move(*args.UsedBlocks);
    }

    StateLoadFinished = true;
    NextVolumeConfigVersion = GetCurrentConfigVersion();
    LastHistoryCleanup = ctx.Now();

    SignalTabletActive(ctx);
    ScheduleProcessUpdateVolumeConfig(ctx);
    ScheduleAllocateDiskIfNeeded(ctx);

    if (State) {
        ProcessNextPendingClientRequest(ctx);
    }
}

}   // namespace NCloud::NBlockStore::NStorage
