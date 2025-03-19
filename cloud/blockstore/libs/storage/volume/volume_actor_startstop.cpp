#include "volume_actor.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/storage/api/bootstrapper.h>
#include <cloud/blockstore/libs/storage/api/partition.h>
#include <cloud/blockstore/libs/storage/bootstrapper/bootstrapper.h>
#include <cloud/blockstore/libs/storage/partition/part.h>
#include <cloud/blockstore/libs/storage/partition2/part2.h>
#include <cloud/blockstore/libs/storage/partition_nonrepl/config.h>
#include <cloud/blockstore/libs/storage/partition_nonrepl/part_mirror.h>
#include <cloud/blockstore/libs/storage/partition_nonrepl/part_nonrepl.h>
#include <cloud/blockstore/libs/storage/partition_nonrepl/part_nonrepl_migration.h>

#include <cloud/storage/core/libs/common/media.h>

#include <ydb/core/base/tablet.h>
#include <ydb/core/tablet/tablet_setup.h>

#include <util/string/builder.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

using namespace NCloud::NBlockStore::NStorage::NPartition;

using namespace NCloud::NStorage;

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::SendBootExternalRequest(
    const TActorContext& ctx,
    TPartitionInfo& partition)
{
    if (partition.Bootstrapper || partition.RequestingBootExternal) {
        return false;
    }

    LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Requesting external boot for tablet",
        partition.TabletId);

    NCloud::Send<TEvHiveProxy::TEvBootExternalRequest>(
        ctx,
        MakeHiveProxyServiceId(),
        partition.TabletId,
        partition.TabletId);

    partition.RetryCookie.Detach();
    partition.RequestingBootExternal = true;
    return true;
}

void TVolumeActor::ScheduleRetryStartPartition(
    const TActorContext& ctx,
    TPartitionInfo& partition)
{
    const TDuration timeout = partition.RetryTimeout;
    partition.RetryTimeout = Min(
        timeout + Config->GetTabletRebootCoolDownIncrement(),
        Config->GetTabletRebootCoolDownMax());

    if (!timeout) {
        // Don't schedule anything, retry immediately
        SendBootExternalRequest(ctx, partition);
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Waiting before retrying start of partition %lu (timeout: %s)",
        TabletID(),
        partition.TabletId,
        ToString(timeout).data());

    partition.RetryCookie.Reset(ISchedulerCookie::Make3Way());
    ctx.Schedule(
        partition.RetryTimeout,
        new TEvVolumePrivate::TEvRetryStartPartition(
            partition.TabletId,
            partition.RetryCookie.Get()),
        partition.RetryCookie.Get());
}

void TVolumeActor::SendPendingRequests(const TActorContext& ctx)
{
    while (!PendingRequests.empty()) {
        ctx.Send(PendingRequests.front().Event.release());
        PendingRequests.pop_front();
    }
}

void TVolumeActor::StartPartitionsIfNeeded(const TActorContext& ctx)
{
    if (!PartitionsStartRequested && State) {
        StartPartitions(ctx);
    }
}

void TVolumeActor::SetupNonreplicatedPartitions(const TActorContext& ctx)
{
    if (State->GetMeta().GetDevices().empty()) {
        return;
    }

    auto nonreplicatedConfig = std::make_shared<TNonreplicatedPartitionConfig>(
        State->GetMeta().GetDevices(),
        State->GetMeta().GetIOMode(),
        State->GetDiskId(),
        State->GetMeta().GetConfig().GetBlockSize(),
        SelfId(),
        State->GetMeta().GetMuteIOErrors(),
        State->GetTrackUsedBlocks(),
        THashSet<TString>(
            State->GetMeta().GetFreshDeviceIds().begin(),
            State->GetMeta().GetFreshDeviceIds().end()));

    TActorId nonreplicatedActorId;

    NRdma::IClientPtr rdmaClient;
    if (Config->GetUseNonreplicatedRdmaActor() && State->GetUseRdma()) {
        rdmaClient = RdmaClient;
    }

    const auto& migrations = State->GetMeta().GetMigrations();
    if (migrations.size()) {
        nonreplicatedActorId = NCloud::Register(
            ctx,
            CreateNonreplicatedPartitionMigration(
                Config,
                ProfileLog,
                BlockDigestGenerator,
                State->GetMeta().GetMigrationIndex(),
                State->GetReadWriteAccessClientId(),
                nonreplicatedConfig,
                migrations,
                std::move(rdmaClient)));
    } else {
        const auto& metaReplicas = State->GetMeta().GetReplicas();

        if (metaReplicas.empty()) {
            nonreplicatedActorId = NCloud::Register(
                ctx,
                CreateNonreplicatedPartition(
                    Config,
                    nonreplicatedConfig,
                    SelfId(),
                    std::move(rdmaClient)));
        } else {
            TVector<TDevices> replicas;
            for (const auto& metaReplica: metaReplicas) {
                replicas.push_back(metaReplica.GetDevices());
            }

            // XXX naming (nonreplicated)
            nonreplicatedActorId = NCloud::Register(
                ctx,
                CreateMirrorPartition(
                    Config,
                    ProfileLog,
                    BlockDigestGenerator,
                    State->GetReadWriteAccessClientId(),
                    nonreplicatedConfig,
                    std::move(replicas),
                    std::move(rdmaClient)));
        }
    }

    State->SetNonreplicatedPartitionActor(
        nonreplicatedActorId,
        std::move(nonreplicatedConfig));
}

void TVolumeActor::StartPartitions(const TActorContext& ctx)
{
    Y_VERIFY(State);

    // Request storage info for partitions
    for (auto& partition: State->GetPartitions()) {
        SendBootExternalRequest(ctx, partition);
    }

    if (IsDiskRegistryMediaKind(State->GetConfig().GetStorageMediaKind())) {
        SetupNonreplicatedPartitions(ctx);

        if (State->Ready()) {
            SendPendingRequests(ctx);
        }
    }

    PartitionsStartRequested = true;
}

void TVolumeActor::StopPartitions(const TActorContext& ctx)
{
    if (!State) {
        return;
    }

    for (auto& part : State->GetPartitions()) {
        // Reset previous boot attempts
        part.RetryCookie.Detach();
        part.RequestingBootExternal = false;
        part.SuggestedGeneration = 0;
        part.StorageInfo = {};

        // Stop any currently booting or running tablet
        if (part.Bootstrapper) {
            NCloud::Send<TEvBootstrapper::TEvStop>(
                ctx,
                part.Bootstrapper);
        }
    }

    if (auto actorId = State->GetNonreplicatedPartitionActor()) {
        LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
            "[%lu] Send poison pill to partition %s",
            TabletID(),
            actorId.ToString().c_str());

        NCloud::Send<TEvents::TEvPoisonPill>(ctx, actorId);
    }
}

void TVolumeActor::HandleRdmaUnavailable(
    const TEvVolume::TEvRdmaUnavailable::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    LOG_WARN(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Rdma unavailable, restarting without rdma",
        TabletID());

    StopPartitions(ctx);
    State->SetRdmaUnavailable();
    StartPartitions(ctx);
}

void TVolumeActor::HandleRetryStartPartition(
    const TEvVolumePrivate::TEvRetryStartPartition::TPtr& ev,
    const TActorContext& ctx)
{
    const auto& msg = ev->Get();

    if (msg->Cookie.DetachEvent()) {
        auto* part = State->GetPartition(msg->TabletId);

        Y_VERIFY(part, "Scheduled retry for missing partition %lu", msg->TabletId);

        part->RetryCookie.Detach();

        SendBootExternalRequest(ctx, *part);
    }
}

void TVolumeActor::HandleBootExternalResponse(
    const TEvHiveProxy::TEvBootExternalResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const ui64 partTabletId = ev->Cookie;

    auto* part = State->GetPartition(partTabletId);

    if (!part || !part->RequestingBootExternal || part->Bootstrapper) {
        LOG_ERROR(ctx, TBlockStoreComponents::VOLUME,
            "[%lu] Received unexpected external boot info for part %lu",
            TabletID(),
            partTabletId);
        return;
    }

    part->RequestingBootExternal = false;

    const auto& error = msg->GetError();
    if (FAILED(error.GetCode())) {
        LOG_ERROR(ctx, TBlockStoreComponents::VOLUME,
            "[%lu] BootExternalRequest for part %lu failed: %s",
            TabletID(),
            partTabletId,
            FormatError(error).data());

        part->SetFailed(TStringBuilder()
            << "BootExternalRequest failed: " << FormatError(error));
        ScheduleRetryStartPartition(ctx, *part);
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Received external boot info for part %lu",
        TabletID(),
        partTabletId);

    if (msg->StorageInfo->TabletType != TTabletTypes::BlockStorePartition &&
        msg->StorageInfo->TabletType != TTabletTypes::BlockStorePartition2) {
        // Partitions use specific tablet factory
        LOG_ERROR_S(ctx, TBlockStoreComponents::VOLUME,
            "[" << TabletID() << "] Unexpected part " << partTabletId
            << " with type " << msg->StorageInfo->TabletType);
        part->SetFailed(
            TStringBuilder()
                << "Unexpected tablet type: "
                << msg->StorageInfo->TabletType);
        // N.B.: this is a fatal error, don't retry
        return;
    }

    Y_VERIFY(msg->StorageInfo->TabletID == partTabletId,
        "Tablet IDs mismatch: %lu vs %lu",
        msg->StorageInfo->TabletID,
        partTabletId);

    part->SuggestedGeneration = msg->SuggestedGeneration;
    part->StorageInfo = msg->StorageInfo;

    LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Starting partition %lu",
        TabletID(),
        partTabletId);

    const auto* appData = AppData(ctx);

    auto config = Config;
    auto partitionConfig = part->PartitionConfig;
    auto diagnosticsConfig = DiagnosticsConfig;
    auto profileLog = ProfileLog;
    auto blockDigestGenerator = BlockDigestGenerator;
    auto storageAccessMode = State->GetStorageAccessMode();
    auto siblingCount = State->GetPartitions().size();
    auto selfId = SelfId();

    auto factory = [=] (const TActorId& owner, TTabletStorageInfo* storage) {
        Y_VERIFY(
            storage->TabletType == TTabletTypes::BlockStorePartition ||
            storage->TabletType == TTabletTypes::BlockStorePartition2);

        if (storage->TabletType == TTabletTypes::BlockStorePartition) {
            return NPartition::CreatePartitionTablet(
                owner,
                storage,
                config,
                diagnosticsConfig,
                profileLog,
                blockDigestGenerator,
                std::move(partitionConfig),
                storageAccessMode,
                siblingCount,
                selfId).release();
        } else {
            return NPartition2::CreatePartitionTablet(
                owner,
                storage,
                config,
                diagnosticsConfig,
                profileLog,
                blockDigestGenerator,
                std::move(partitionConfig),
                storageAccessMode,
                siblingCount,
                selfId).release();
        }
    };

    auto setupInfo = MakeIntrusive<TTabletSetupInfo>(
        factory,
        TMailboxType::ReadAsFilled,
        appData->UserPoolId,
        TMailboxType::ReadAsFilled,
        appData->SystemPoolId);

    TBootstrapperConfig bootConfig;
    bootConfig.SuggestedGeneration = part->SuggestedGeneration;
    bootConfig.BootAttemptsThreshold = 1;

    auto bootstrapper = NCloud::RegisterLocal(ctx, CreateBootstrapper(
        bootConfig,
        SelfId(),
        part->StorageInfo,
        std::move(setupInfo)));

    part->Init(bootstrapper);

    NCloud::Send<TEvBootstrapper::TEvStart>(ctx, bootstrapper);
}

void TVolumeActor::HandleTabletStatus(
    const TEvBootstrapper::TEvStatus::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto* partition = State->GetPartition(msg->TabletId);
    Y_VERIFY(partition, "Missing partition state for %lu", msg->TabletId);

    if (partition->Bootstrapper != ev->Sender) {
        // CompleteUpdateConfig calls StopPartitions, and then it
        // calls StartPartitions with a completely new state.
        // Ignore any signals from outdated bootstrappers.
        return;
    }

    switch (msg->Status) {
        case TEvBootstrapper::STARTED:
            partition->RetryTimeout = TDuration::Zero();
            partition->SetStarted(msg->TabletUser);
            Y_VERIFY(State->SetPartitionStatActor(msg->TabletId, msg->TabletUser));
            NCloud::Send<TEvPartition::TEvWaitReadyRequest>(
                ctx,
                msg->TabletUser,
                msg->TabletId);
            break;
        case TEvBootstrapper::STOPPED:
            partition->Bootstrapper = {};
            partition->SetStopped();
            break;
        case TEvBootstrapper::RACE:
            // Retry immediately when hive generation is out of sync
            partition->RetryTimeout = TDuration::Zero();
            /* fall through */
        case TEvBootstrapper::FAILED:
            partition->Bootstrapper = {};
            partition->SetFailed(msg->Message);
            if (partition->StorageInfo) {
                partition->StorageInfo = {};
                partition->SuggestedGeneration = 0;
                ScheduleRetryStartPartition(ctx, *partition);
            }
            break;
    }

    auto oldStatus = GetVolumeStatus();
    auto state = State->UpdatePartitionsState();
    auto newStatus = GetVolumeStatus();

    if (oldStatus != newStatus) {
        DumpUsageStats(ctx, oldStatus);
    }

    if (state == TPartitionInfo::READY) {
        // All partitions ready, it's time to reply to requests
        SendPendingRequests(ctx);
    }
}

void TVolumeActor::HandleWaitReadyResponse(
    const TEvPartition::TEvWaitReadyResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const ui64 tabletId = ev->Cookie;

    auto* partition = State->GetPartition(tabletId);

    // Drop unexpected responses in case of restart races
    if (partition &&
        partition->State == TPartitionInfo::STARTED &&
        partition->Owner == ev->Sender)
    {
        partition->SetReady();

        auto oldStatus = GetVolumeStatus();
        auto state = State->UpdatePartitionsState();
        auto newStatus = GetVolumeStatus();

        if (oldStatus != newStatus) {
            DumpUsageStats(ctx, oldStatus);
            UsageIntervalBegin = ctx.Now();
        }

        if (state == TPartitionInfo::READY) {
            // All partitions ready, it's time to reply to requests
            SendPendingRequests(ctx);
            ProcessNextCheckpointRequest(ctx);
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
