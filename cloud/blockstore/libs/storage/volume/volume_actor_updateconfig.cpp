#include "volume_actor.h"

#include "volume_database.h"

#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/storage/core/config.h>

#include <cloud/storage/core/libs/common/media.h>

#include <library/cpp/protobuf/util/pb_io.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

auto BuildNewMeta(
    const NKikimrBlockStore::TUpdateVolumeConfig& msg,
    const NProto::TVolumeMeta& oldMeta)
{
    const NProto::TPartitionConfig& oldConfig = oldMeta.GetConfig();

    const auto& volumeConfig = msg.GetVolumeConfig();
    const auto configVersion = volumeConfig.GetVersion();
    const auto mediaKind = static_cast<NCloud::NProto::EStorageMediaKind>(
        volumeConfig.GetStorageMediaKind());

    NProto::TVolumeMeta newMeta;

    newMeta.SetIOMode(oldMeta.GetIOMode());
    newMeta.SetIOModeTs(oldMeta.GetIOModeTs());

    Y_VERIFY(volumeConfig.PartitionsSize());
    ui64 blockCount = volumeConfig.GetPartitions(0).GetBlockCount();

    auto& partitionConfig = *newMeta.MutableConfig();

    partitionConfig.CopyFrom(oldConfig);
    partitionConfig.SetProjectId(volumeConfig.GetProjectId());
    partitionConfig.SetFolderId(volumeConfig.GetFolderId());
    partitionConfig.SetCloudId(volumeConfig.GetCloudId());
    partitionConfig.SetTabletVersion(volumeConfig.GetTabletVersion());

    partitionConfig.SetDiskId(volumeConfig.GetDiskId());
    partitionConfig.SetBaseDiskId(volumeConfig.GetBaseDiskId());
    partitionConfig.SetBaseDiskCheckpointId(volumeConfig.GetBaseDiskCheckpointId());
    partitionConfig.SetBlocksCount(blockCount);
    partitionConfig.SetBlockSize(volumeConfig.GetBlockSize());
    partitionConfig.SetMaxBlocksInBlob(volumeConfig.GetMaxBlocksInBlob());
    partitionConfig.SetZoneBlockCount(volumeConfig.GetZoneBlockCount());
    partitionConfig.SetStorageMediaKind(mediaKind);
    while (partitionConfig.ExplicitChannelProfilesSize()
            < volumeConfig.ExplicitChannelProfilesSize())
    {
        partitionConfig.AddExplicitChannelProfiles();
    }
    for (ui32 i = 0; i < volumeConfig.ExplicitChannelProfilesSize(); ++i) {
        const auto& src = volumeConfig.GetExplicitChannelProfiles(i);
        auto* dst = partitionConfig.MutableExplicitChannelProfiles(i);
        dst->SetDataKind(src.GetDataKind());
        dst->SetPoolKind(src.GetPoolKind());
    }
    auto* pp = partitionConfig.MutablePerformanceProfile();
    pp->SetMaxReadBandwidth(
        volumeConfig.GetPerformanceProfileMaxReadBandwidth());
    pp->SetMaxWriteBandwidth(
        volumeConfig.GetPerformanceProfileMaxWriteBandwidth());
    pp->SetMaxReadIops(volumeConfig.GetPerformanceProfileMaxReadIops());
    pp->SetMaxWriteIops(volumeConfig.GetPerformanceProfileMaxWriteIops());
    pp->SetBurstPercentage(volumeConfig.GetPerformanceProfileBurstPercentage());
    pp->SetMaxPostponedWeight(
        volumeConfig.GetPerformanceProfileMaxPostponedWeight());
    pp->SetBoostTime(volumeConfig.GetPerformanceProfileBoostTime());
    pp->SetBoostRefillTime(volumeConfig.GetPerformanceProfileBoostRefillTime());
    pp->SetBoostPercentage(volumeConfig.GetPerformanceProfileBoostPercentage());
    pp->SetThrottlingEnabled(
        volumeConfig.GetPerformanceProfileThrottlingEnabled());

    newMeta.MutableVolumeConfig()->CopyFrom(volumeConfig);
    newMeta.SetVersion(configVersion);

    // Replace the list of partitions
    newMeta.ClearPartitions();
    newMeta.MutablePartitions()->Resize(msg.PartitionsSize(), 0);

    for (const auto& partition: msg.GetPartitions()) {
        ui32 partitionId = partition.GetPartitionId();
        Y_VERIFY(partitionId < volumeConfig.PartitionsSize());

        ui64 tabletId = partition.GetTabletId();
        Y_VERIFY(tabletId);

        newMeta.SetPartitions(partitionId, tabletId);
    }

    for (ui64 tabletId: newMeta.GetPartitions()) {
        Y_VERIFY(tabletId);
    }

    return newMeta;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

ui32 TVolumeActor::GetCurrentConfigVersion() const
{
    if (State) {
        return State->GetMeta().GetVersion();
    }
    return 0;
}

void TVolumeActor::HandleUpdateVolumeConfig(
    const TEvBlockStore::TEvUpdateVolumeConfig::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    ui32 configVersion = msg->Record.GetVolumeConfig().GetVersion();
    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Received volume config with version %u",
        TabletID(),
        configVersion);

    if (!StateLoadFinished || ProcessUpdateVolumeConfigScheduled) {
        // will be processed later
        PendingVolumeConfigUpdates.emplace_back(ev.Release());
        ScheduleProcessUpdateVolumeConfig(ctx);
        return;
    }

    UpdateVolumeConfig(ev, ctx);
}

bool TVolumeActor::UpdateVolumeConfig(
    const TEvBlockStore::TEvUpdateVolumeConfig::TPtr& ev,
    const TActorContext& ctx)
{
    Y_VERIFY(StateLoadFinished);
    UpdateVolumeConfigInProgress = true;

    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>(),
        std::move(ev->TraceId));

    ui32 configVersion = msg->Record.GetVolumeConfig().GetVersion();
    if (configVersion <= NextVolumeConfigVersion) {
        auto response = std::make_unique<TEvBlockStore::TEvUpdateVolumeConfigResponse>();
        response->Record.SetTxId(msg->Record.GetTxId());
        response->Record.SetOrigin(TabletID());

        if (configVersion == NextVolumeConfigVersion) {
            response->Record.SetStatus(NKikimrBlockStore::OK);
        } else {
            response->Record.SetStatus(NKikimrBlockStore::ERROR_BAD_VERSION);
        }

        NCloud::Reply(ctx, *requestInfo, std::move(response));
        UpdateVolumeConfigInProgress = false;
        return false;
    }

    const auto mediaKind = static_cast<NCloud::NProto::EStorageMediaKind>(
        msg->Record.GetVolumeConfig().GetStorageMediaKind());

    UnfinishedUpdateVolumeConfig.Record = std::move(msg->Record);
    UnfinishedUpdateVolumeConfig.ConfigVersion = configVersion;
    UnfinishedUpdateVolumeConfig.RequestInfo = std::move(requestInfo);

    if (IsDiskRegistryMediaKind(mediaKind)) {
        LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
            "[%lu] Allocating disk", TabletID());
        AllocateDisk(ctx);
    } else {
        FinishUpdateVolumeConfig(ctx);
    }

    return true;
}

void TVolumeActor::ScheduleProcessUpdateVolumeConfig(const TActorContext& ctx)
{
    if (PendingVolumeConfigUpdates && !ProcessUpdateVolumeConfigScheduled) {
        ctx.Send(ctx.SelfID, new TEvVolumePrivate::TEvProcessUpdateVolumeConfig());
        ProcessUpdateVolumeConfigScheduled = true;
    }
}

void TVolumeActor::HandleProcessUpdateVolumeConfig(
    const TEvVolumePrivate::TEvProcessUpdateVolumeConfig::TPtr&,
    const TActorContext& ctx)
{
    Y_VERIFY(ProcessUpdateVolumeConfigScheduled);
    ProcessUpdateVolumeConfigScheduled = false;

    if (!StateLoadFinished) {
        ScheduleProcessUpdateVolumeConfig(ctx);
        return;
    }

    while (PendingVolumeConfigUpdates) {
        auto ev = std::move(PendingVolumeConfigUpdates.front());
        PendingVolumeConfigUpdates.pop_front();

        if (UpdateVolumeConfig(ev.release(), ctx)) {
            // should wait for tx completion
            break;
        }
    }
}

void TVolumeActor::FinishUpdateVolumeConfig(const TActorContext& ctx)
{
    auto newMeta = BuildNewMeta(
        UnfinishedUpdateVolumeConfig.Record,
        State ? State->GetMeta() : NProto::TVolumeMeta()
    );

    *newMeta.MutableDevices() = std::move(UnfinishedUpdateVolumeConfig.Devices);
    *newMeta.MutableMigrations() = std::move(UnfinishedUpdateVolumeConfig.Migrations);
    newMeta.ClearReplicas();
    for (auto& devices: UnfinishedUpdateVolumeConfig.Replicas) {
        auto* replica = newMeta.AddReplicas();
        *replica->MutableDevices() = std::move(devices);
    }
    newMeta.ClearFreshDeviceIds();
    for (auto& freshDeviceId: UnfinishedUpdateVolumeConfig.FreshDeviceIds) {
        *newMeta.AddFreshDeviceIds() = std::move(freshDeviceId);
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Updating volume config to version %u",
        TabletID(),
        UnfinishedUpdateVolumeConfig.ConfigVersion);

    Y_VERIFY(NextVolumeConfigVersion == GetCurrentConfigVersion());
    NextVolumeConfigVersion = UnfinishedUpdateVolumeConfig.ConfigVersion;

    ExecuteTx<TUpdateConfig>(
        ctx,
        std::move(UnfinishedUpdateVolumeConfig.RequestInfo),
        UnfinishedUpdateVolumeConfig.Record.GetTxId(),
        std::move(newMeta));

    UnfinishedUpdateVolumeConfig.Clear();
}

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::PrepareUpdateConfig(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TUpdateConfig& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);

    Y_VERIFY(args.Meta.GetVersion() == NextVolumeConfigVersion);

    return true;
}

void TVolumeActor::ExecuteUpdateConfig(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TUpdateConfig& args)
{
    Y_UNUSED(ctx);

    TVolumeDatabase db(tx.DB);
    if (State) {
        db.WriteHistory(State->LogUpdateMeta(ctx.Now(), args.Meta));
    }
    db.WriteMeta(args.Meta);
}

void TVolumeActor::CompleteUpdateConfig(
    const TActorContext& ctx,
    TTxVolume::TUpdateConfig& args)
{
    Y_VERIFY(args.Meta.GetVersion() == NextVolumeConfigVersion);

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Sending OK response for UpdateVolumeConfig with version=%u",
        TabletID(),
        args.Meta.GetVersion());

    auto response = std::make_unique<TEvBlockStore::TEvUpdateVolumeConfigResponse>();
    response->Record.SetTxId(args.TxId);
    response->Record.SetOrigin(TabletID());
    response->Record.SetStatus(NKikimrBlockStore::OK);

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

    if (State) {
        DumpUsageStats(ctx, GetVolumeStatus());

        if (auto actorId = State->GetNonreplicatedPartitionActor()) {
            WaitForPartitions.emplace_back(actorId, nullptr);
        }
    }

    // stop partitions that might have been using old configuration
    StopPartitions(ctx);

    if (State) {
        State->ResetMeta(args.Meta);
        SendVolumeConfigUpdated(ctx);
    } else {
        TThrottlerConfig throttlerConfig(
            Config->GetMaxThrottlerDelay(),
            Config->GetMaxWriteCostMultiplier(),
            Config->GetDefaultPostponedRequestWeight(),
            CalculateBoostTime(args.Meta.GetConfig().GetPerformanceProfile())
        );

        State.reset(new TVolumeState(
            Config,
            args.Meta,
            throttlerConfig,
            {},  // clients
            {},  // history
            {}   // checkpoint requests
        ));
        RegisterVolume(ctx);
    }

    Y_VERIFY(NextVolumeConfigVersion == GetCurrentConfigVersion());

    if (CurrentState == STATE_INIT) {
        BecomeAux(ctx, STATE_WORK);
    }

    LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] State initialization finished",
        TabletID());

    StartPartitions(ctx);
    ResetServicePipes(ctx);
    ScheduleProcessUpdateVolumeConfig(ctx);
    ScheduleAllocateDiskIfNeeded(ctx);
    UpdateVolumeConfigInProgress = false;
}

}   // namespace NCloud::NBlockStore::NStorage
