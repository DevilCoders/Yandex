#include "disk_registry_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleBackupDiskRegistryState(
    const TEvDiskRegistry::TEvBackupDiskRegistryStateRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(BackupDiskRegistryState);

    const auto* msg = ev->Get();

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received BackupDiskRegistryState request. BackupLocalDB=%s",
        TabletID(),
        msg->Record.GetBackupLocalDB() ? "true" : "false");

    if (!msg->Record.GetBackupLocalDB()) {
        auto response = std::make_unique<TEvDiskRegistry::TEvBackupDiskRegistryStateResponse>();
        *response->Record.MutableBackup() = State->BackupState();

        NCloud::Reply(ctx, *ev, std::move(response));

        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    ExecuteTx<TBackupDiskRegistryState>(ctx, std::move(requestInfo));
}

////////////////////////////////////////////////////////////////////////////////

bool TDiskRegistryActor::PrepareBackupDiskRegistryState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TBackupDiskRegistryState& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);
    return LoadState(db, args);
}

void TDiskRegistryActor::ExecuteBackupDiskRegistryState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TBackupDiskRegistryState& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TDiskRegistryActor::CompleteBackupDiskRegistryState(
    const TActorContext& ctx,
    TTxDiskRegistry::TBackupDiskRegistryState& args)
{
    auto response = std::make_unique<TEvDiskRegistry::TEvBackupDiskRegistryStateResponse>();

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
    ] = static_cast<TTxDiskRegistry::TLoadDBState&>(args);

    auto copy = [] (auto& src, auto* dst) {
        dst->Reserve(src.size());
        std::copy(
            std::make_move_iterator(src.begin()),
            std::make_move_iterator(src.end()),
            RepeatedFieldBackInserter(dst)
        );
        src.clear();
    };

    auto transform = [] (auto& src, auto* dst, auto func) {
        dst->Reserve(src.size());
        for (auto& x: src) {
            func(x, *dst->Add());
        }
        src.clear();
    };

    auto& backup = *response->Record.MutableBackup();

    copy(disks, backup.MutableDisks());
    copy(dirtyDevices, backup.MutableDirtyDevices());
    copy(suspendedDevices, backup.MutableSuspendedDevices());

    Y_UNUSED(oldAgents);
    copy(agents, backup.MutableAgents());

    copy(placementGroups, backup.MutablePlacementGroups());
    copy(disksToNotify, backup.MutableDisksToNotify());
    copy(disksToCleanup, backup.MutableDisksToCleanup());
    copy(errorNotifications, backup.MutableErrorNotifications());
    copy(outdatedVolumeConfigs, backup.MutableOutdatedVolumeConfigs());

    transform(brokenDisks, backup.MutableBrokenDisks(), [] (auto& src, auto& dst) {
        dst.SetDiskId(src.DiskId);
        dst.SetTsToDestroy(src.TsToDestroy.MicroSeconds());
    });

    transform(diskStateChanges, backup.MutableDiskStateChanges(), [] (auto& src, auto& dst) {
        dst.MutableState()->Swap(&src.State);
        dst.SetSeqNo(src.SeqNo);
    });

    backup.MutableConfig()->Swap(&config);
    backup.MutableConfig()->SetLastDiskStateSeqNo(lastDiskStateSeqNo);
    backup.MutableConfig()->SetDiskAllocationAllowed(diskAllocationAllowed);

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
