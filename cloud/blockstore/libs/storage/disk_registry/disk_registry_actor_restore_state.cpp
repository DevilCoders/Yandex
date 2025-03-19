#include "disk_registry_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

TTxDiskRegistry::TLoadDBState MakeNewLoadState(
    NProto::TDiskRegistryStateBackup&& backup)
{
    auto move = [] (auto& src, auto& dst) {
        dst.reserve(src.size());
        dst.assign(
            std::make_move_iterator(src.begin()),
            std::make_move_iterator(src.end()));
        src.Clear();
    };

    auto transform = [] (auto& src, auto& dst, auto func) {
        dst.resize(src.size());
        for (int i = 0; i < src.size(); ++i) {
            func(src[i], dst[i]);
        }
        src.Clear();
    };

    TTxDiskRegistry::TLoadDBState newLoadState;
    // if new fields are added to TLoadDBState there will be a compilation error.
    auto& [
        config,
        dirtyDevices,
        oldAgents,
        agents,
        disks,
        placementGroups,
        brokenDisks,
        disksToNotify,
        diskStateChanges,
        lastDiskStateSeqNo,
        diskAllocationAllowed,
        disksToCleanup,
        errorNotifications,
        outdatedVolumeConfigs,
        suspendedDevices
    ] = static_cast<TTxDiskRegistry::TLoadDBState&>(newLoadState);

    move(*backup.MutableDirtyDevices(), dirtyDevices);
    move(*backup.MutableAgents(), agents);
    move(*backup.MutableDisks(), disks);
    move(*backup.MutablePlacementGroups(), placementGroups);
    move(*backup.MutableDisksToNotify(), disksToNotify);
    move(*backup.MutableDisksToCleanup(), disksToCleanup);
    move(*backup.MutableErrorNotifications(), errorNotifications);
    move(*backup.MutableOutdatedVolumeConfigs(), outdatedVolumeConfigs);
    move(*backup.MutableSuspendedDevices(), suspendedDevices);

    transform(
        *backup.MutableBrokenDisks(),
        brokenDisks,
        [] (auto& src, auto& dst) {
            dst.DiskId = src.GetDiskId();
            dst.TsToDestroy = TInstant::MicroSeconds(src.GetTsToDestroy());
        });
    transform(
        *backup.MutableDiskStateChanges(),
        diskStateChanges,
        [] (auto& src, auto& dst) {
            if (src.HasState()) {
                dst.State.Swap(src.MutableState());
            }
            dst.SeqNo = src.GetSeqNo();
        });

    if (backup.HasConfig()) {
        config.Swap(backup.MutableConfig());
    }

    lastDiskStateSeqNo = config.GetLastDiskStateSeqNo();
    diskAllocationAllowed = config.GetDiskAllocationAllowed();

    return newLoadState;
}

bool NormalizeLoadState(
    const TActorContext& ctx,
    TTxDiskRegistry::TLoadDBState& state)
{
    auto sortAndTestUnique = [&ctx] (auto& container, auto&& getKeyFunction) {
        SortBy(container, getKeyFunction);

        auto itr = std::adjacent_find(
            container.begin(),
            container.end(),
            [&] (auto&& left, auto&& right) {
                return getKeyFunction(left) == getKeyFunction(right);
            });

        if (itr != container.end()) {
            LOG_WARN(
                ctx,
                TBlockStoreComponents::DISK_REGISTRY,
                "Not unique: %s",
                ToString(getKeyFunction(*itr)).c_str());
            return false;
        }

        return true;
    };

    // if new fields are added to TLoadDBState there will be a compilation error.
    auto& [
        config,
        dirtyDevices,
        oldAgents,
        agents,
        disks,
        placementGroups,
        brokenDisks,
        disksToNotify,
        diskStateChanges,
        lastDiskStateSeqNo,
        diskAllocationAllowed,
        disksToCleanup,
        errorNotifications,
        outdatedVolumeConfigs,
        suspendedDevices
    ] = state;

    Y_UNUSED(oldAgents);
    Y_UNUSED(lastDiskStateSeqNo);
    Y_UNUSED(diskAllocationAllowed);

    bool result = true;

    result &= sortAndTestUnique(
        *config.MutableKnownAgents(),
        [] (const auto& config) {
            return config.GetAgentId();
        });

    for (auto& agent: *state.Config.MutableKnownAgents()) {
        result &= sortAndTestUnique(
            *agent.MutableDevices(),
            [] (const auto& config) {
                return config.GetDeviceUUID();
            });
    }

    result &= sortAndTestUnique(
        dirtyDevices,
        [] (const auto& device) {
            return device;
        });

    result &= sortAndTestUnique(
        agents,
        [] (const auto& config) {
            return config.GetAgentId();
        });

    for (auto& agent: agents) {
        result &= sortAndTestUnique(
            *agent.MutableDevices(),
            [] (const auto& config) {
                return config.GetDeviceUUID();
            });
    }

    result &= sortAndTestUnique(
        disks,
        [] (const auto& config) {
            return config.GetDiskId();
        });

    result &= sortAndTestUnique(
        placementGroups,
        [] (const auto& config) {
            return config.GetGroupId();
        });

    result &= sortAndTestUnique(
        brokenDisks,
        [] (const auto& disk) {
            return disk.DiskId;
        });

    result &= sortAndTestUnique(
        disksToNotify,
        [] (const auto& disk) {
            return disk;
        });

    result &= sortAndTestUnique(
        diskStateChanges,
        [] (const auto& disk) {
            return disk.State.GetDiskId();
        });

    result &= sortAndTestUnique(
        disksToCleanup,
        [] (const auto& disk) {
            return disk;
        });

    result &= sortAndTestUnique(
        errorNotifications,
        [] (const auto& disk) {
            return disk;
        });

    result &= sortAndTestUnique(
        outdatedVolumeConfigs,
        [] (const auto& disk) {
            return disk;
        });

    result &= sortAndTestUnique(
        suspendedDevices,
        [] (const auto& device) {
            return device;
        });

    return result;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleRestoreDiskRegistryState(
    const TEvDiskRegistry::TEvRestoreDiskRegistryStateRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(RestoreDiskRegistryState);

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received RestoreDiskRegistryState request",
        TabletID());

    auto* msg = ev->Get();
    if (!msg->Record.HasBackup()) {
        auto response = std::make_unique<
            TEvDiskRegistry::TEvRestoreDiskRegistryStateResponse>();
        *response->Record.MutableError()
            = MakeError(E_ARGUMENT, "No backup in request");
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    auto loadState = MakeNewLoadState(
        std::move(*msg->Record.MutableBackup()));

    if (!NormalizeLoadState(ctx, loadState)) {
        auto response = std::make_unique<
            TEvDiskRegistry::TEvRestoreDiskRegistryStateResponse>();
        *response->Record.MutableError()
            = MakeError(E_ARGUMENT, "Normalize new state error");
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    BecomeAux(ctx, STATE_RESTORE);

    ExecuteTx<TRestoreDiskRegistryState>(
        ctx,
        std::move(requestInfo),
        std::move(loadState));
}

////////////////////////////////////////////////////////////////////////////////

bool TDiskRegistryActor::PrepareRestoreDiskRegistryState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TRestoreDiskRegistryState& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);
    return LoadState(db, args.CurrentState);
}

void TDiskRegistryActor::ExecuteRestoreDiskRegistryState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TRestoreDiskRegistryState& args)
{
    Y_UNUSED(ctx);

    auto& [
        newConfig,
        newDirtyDevices,
        newOldAgents,
        newAgents,
        newDisks,
        newPlacementGroups,
        newBrokenDisks,
        newDisksToNotify,
        newDiskStateChanges,
        newLastDiskStateSeqNo,
        newDiskAllocationAllowed,
        newDisksToCleanup,
        newErrorNotifications,
        newOutdatedVolumeConfigs,
        newSuspendedDevices
    ] = args.NewState;

    auto& [
        currentConfig,
        currentDirtyDevices,
        currentOldAgents,
        currentAgents,
        currentDisks,
        currentPlacementGroups,
        currentBrokenDisks,
        currentDisksToNotify,
        currentDiskStateChanges,
        currentLastDiskStateSeqNo,
        currentDiskAllocationAllowed,
        currentDisksToCleanup,
        currentErrorNotifications,
        currentOutdatedVolumeConfigs,
        currentSuspendedDevices
    ] = args.CurrentState;

    TDiskRegistryDatabase db(tx.DB);

    //Config
    Y_UNUSED(currentConfig);
    db.WriteDiskRegistryConfig(newConfig);

    //DirtyDevices
    for (const auto& disk: currentDirtyDevices) {
        db.DeleteDirtyDevice(disk);
    }
    for (const auto& disk: newDirtyDevices) {
        db.UpdateDirtyDevice(disk);
    }

    //OldAgents
    for (const auto& agent: currentOldAgents) {
        db.DeleteOldAgent(agent.GetNodeId());
    }
    Y_UNUSED(newOldAgents);

    //Agents
    for (const auto& agent: currentAgents) {
        db.DeleteAgent(agent.GetAgentId());
    }
    for (const auto& agent: newAgents) {
        db.UpdateAgent(agent);
    }

    //Disks
    for (const auto& disk: currentDisks) {
        db.DeleteDisk(disk.GetDiskId());
    }
    for (const auto& disk: newDisks) {
        db.UpdateDisk(disk);
    }

    //PlacementGroups
    for (const auto& group: currentPlacementGroups) {
        db.DeletePlacementGroup(group.GetGroupId());
    }
    for (const auto& group: newPlacementGroups) {
        db.UpdatePlacementGroup(group);
    }

    //BrokenDisks
    for (const auto& disk: currentBrokenDisks) {
        db.DeleteBrokenDisk(disk.DiskId);
    }
    for (const auto& disk: newBrokenDisks) {
        db.AddBrokenDisk(disk);
    }

    //BrokenDisks
    for (const auto& disk: currentDisksToNotify) {
        db.DeleteDiskToNotify(disk);
    }
    for (const auto& disk: newDisksToNotify) {
        db.AddDiskToNotify(disk);
    }

    //DiskStateChanges
    for (const auto& disk: currentDiskStateChanges) {
        db.DeleteDiskStateChanges(disk.State.GetDiskId(), disk.SeqNo);
    }
    for (const auto& disk: newDiskStateChanges) {
        db.UpdateDiskState(disk.State, disk.SeqNo);
    }

    //LastDiskStateSeqNo
    Y_UNUSED(currentLastDiskStateSeqNo);
    db.WriteLastDiskStateSeqNo(newLastDiskStateSeqNo);

    //DiskAllocationAllowed
    Y_UNUSED(currentDiskAllocationAllowed);
    db.WriteDiskAllocationAllowed(newDiskAllocationAllowed);

    //DisksToCleanup
    for (const auto& disk: currentDisksToCleanup) {
        db.DeleteDiskToCleanup(disk);
    }
    for (const auto& disk: newDisksToCleanup) {
        db.AddDiskToCleanup(disk);
    }

    //ErrorNotifications
    for (const auto& disk: currentErrorNotifications) {
        db.DeleteErrorNotification(disk);
    }
    for (const auto& disk: newErrorNotifications) {
        db.AddErrorNotification(disk);
    }

    //OutdatedVolumeConfigs
    for (const auto& disk: currentOutdatedVolumeConfigs) {
        db.DeleteOutdatedVolumeConfig(disk);
    }
    for (const auto& disk: newOutdatedVolumeConfigs) {
        db.AddOutdatedVolumeConfig(disk);
    }

    //SuspendedDevices
    for (const auto& disk: currentSuspendedDevices) {
        db.DeleteSuspendedDevice(disk);
    }
    for (const auto& disk: newSuspendedDevices) {
        db.UpdateSuspendedDevice(disk);
    }

}

void TDiskRegistryActor::CompleteRestoreDiskRegistryState(
    const TActorContext& ctx,
    TTxDiskRegistry::TRestoreDiskRegistryState& args)
{
    State.reset(new TDiskRegistryState(
        Config,
        ComponentGroup,
        args.NewState.Config,
        args.NewState.Agents,
        args.NewState.Disks,
        args.NewState.PlacementGroups,
        args.NewState.BrokenDisks,
        args.NewState.DisksToNotify,
        args.NewState.DiskStateChanges,
        args.NewState.LastDiskStateSeqNo,
        args.NewState.DirtyDevices,
        args.NewState.DiskAllocationAllowed,
        args.NewState.DisksToCleanup,
        args.NewState.ErrorNotifications,
        args.NewState.OutdatedVolumeConfigs,
        args.NewState.SuspendedDevices
    ));

    NCloud::Reply(
        ctx,
        *args.RequestInfo,
        std::make_unique<
            TEvDiskRegistry::TEvRestoreDiskRegistryStateResponse>());
}

}   // namespace NCloud::NBlockStore::NStorage
