#include "disk_registry_actor.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleAllocateDisk(
    const TEvDiskRegistry::TEvAllocateDiskRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(AllocateDisk);

    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received AllocateDisk request: DiskId=%s, BlockSize=%u"
        ", BlocksCount=%lu, ReplicaCount=%u, CloudId=%s FolderId=%s",
        TabletID(),
        msg->Record.GetDiskId().c_str(),
        msg->Record.GetBlockSize(),
        msg->Record.GetBlocksCount(),
        msg->Record.GetReplicaCount(),
        msg->Record.GetCloudId().c_str(),
        msg->Record.GetFolderId().c_str());

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    Y_VERIFY_DEBUG(
        msg->Record.GetStorageMediaKind() != NProto::STORAGE_MEDIA_DEFAULT);

    ExecuteTx<TAddDisk>(
        ctx,
        std::move(requestInfo),
        msg->Record.GetDiskId(),
        msg->Record.GetCloudId(),
        msg->Record.GetFolderId(),
        msg->Record.GetPlacementGroupId(),
        msg->Record.GetBlockSize(),
        msg->Record.GetBlocksCount(),
        msg->Record.GetReplicaCount(),
        TVector<TString> {
            msg->Record.GetAgentIds().begin(),
            msg->Record.GetAgentIds().end()
        },
        msg->Record.GetPoolName(),
        msg->Record.GetStorageMediaKind());
}

bool TDiskRegistryActor::PrepareAddDisk(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TAddDisk& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteAddDisk(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TAddDisk& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);

    TDiskRegistryState::TAllocateDiskResult result{};

    args.Error = State->AllocateDisk(
        ctx.Now(),
        db,
        TDiskRegistryState::TAllocateDiskParams {
            .DiskId = args.DiskId,
            .CloudId = args.CloudId,
            .FolderId = args.FolderId,
            .PlacementGroupId = args.PlacementGroupId,
            .BlockSize = args.BlockSize,
            .BlocksCount = args.BlocksCount,
            .ReplicaCount = args.ReplicaCount,
            .AgentIds = args.AgentIds,
            .PoolName = args.PoolName,
            .MediaKind = args.MediaKind
        },
        &result);

    args.Devices = std::move(result.Devices);
    args.DeviceMigration = std::move(result.Migrations);
    args.Replicas = std::move(result.Replicas);
    args.DeviceReplacementUUIDs = std::move(result.DeviceReplacementIds);
    args.IOMode = result.IOMode;
    args.IOModeTs = result.IOModeTs;
    args.MuteIOErrors = result.MuteIOErrors;
}

void TDiskRegistryActor::CompleteAddDisk(
    const TActorContext& ctx,
    TTxDiskRegistry::TAddDisk& args)
{
    auto response = std::make_unique<TEvDiskRegistry::TEvAllocateDiskResponse>();

    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "[%lu] AddDisk error: %s. DiskId=%s",
            TabletID(),
            FormatError(args.Error).c_str(),
            args.DiskId.Quote().c_str());

        *response->Record.MutableError() = std::move(args.Error);
    } else {
        auto outputDevices = [] (const auto& devices, TStringBuilder& result) {
            result << "[";
            if (!devices.empty()) {
                result << " ";
            }
            for (const auto& device: devices) {
                result << "("
                    << device.GetDeviceUUID() << " "
                    << device.GetBlocksCount() << " "
                    << "(" << device.GetUnadjustedBlockCount() << ") "
                    << device.GetBlockSize()
                << ") ";
            }
            result << "]";
        };

        TStringBuilder devices;
        outputDevices(args.Devices, devices);

        TStringBuilder replicas;
        replicas << "[";
        if (!args.Replicas.empty()) {
            replicas << " ";
        }
        for (const auto& replica: args.Replicas) {
            outputDevices(replica, replicas);
        }
        replicas << "]";

        LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "[%lu] AddDisk success. DiskId=%s Devices=%s Replicas=%s",
            TabletID(),
            args.DiskId.Quote().c_str(),
            devices.c_str(),
            replicas.c_str()
        );

        auto onDevice = [&] (NProto::TDeviceConfig& d, ui32 blockSize) {
            if (ToLogicalBlocks(d, blockSize)) {
                return true;
            }

            TStringBuilder error;
            error << "CompleteAddDisk: ToLogicalBlocks failed, device: "
                << d.GetDeviceUUID().Quote().c_str();
            LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY, error);

            *response->Record.MutableError() = MakeError(E_FAIL, error);
            NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

            return false;
        };

        for (auto& device: args.Devices) {
            if (!onDevice(device, args.BlockSize)) {
                return;
            }

            *response->Record.AddDevices() = std::move(device);
        }
        for (auto& m: args.DeviceMigration) {
            if (!onDevice(*m.MutableTargetDevice(), args.BlockSize)) {
                return;
            }

            *response->Record.AddMigrations() = std::move(m);
        }
        for (auto& replica: args.Replicas) {
            auto* r = response->Record.AddReplicas();

            for (auto& device: replica) {
                if (!onDevice(device, args.BlockSize)) {
                    return;
                }

                *r->AddDevices() = std::move(device);
            }
        }

        for (auto& deviceId: args.DeviceReplacementUUIDs) {
            *response->Record.AddDeviceReplacementUUIDs() = std::move(deviceId);
        }

        response->Record.SetIOMode(args.IOMode);
        response->Record.SetIOModeTs(args.IOModeTs.MicroSeconds());
        response->Record.SetMuteIOErrors(args.MuteIOErrors);
    }

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

    DestroyBrokenDisks(ctx);
    NotifyUsers(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleDeallocateDisk(
    const TEvDiskRegistry::TEvDeallocateDiskRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(DeallocateDisk);

    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received DeallocateDisk request: DiskId=%s Force=%d",
        TabletID(),
        msg->Record.GetDiskId().c_str(),
        msg->Record.GetForce());

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    ExecuteTx<TRemoveDisk>(
        ctx,
        std::move(requestInfo),
        msg->Record.GetDiskId(),
        msg->Record.GetForce());
}

bool TDiskRegistryActor::PrepareRemoveDisk(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TRemoveDisk& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteRemoveDisk(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TRemoveDisk& args)
{
    Y_UNUSED(ctx);

    if (!args.Force && !State->IsReadyForCleanup(args.DiskId)) {
        ReportNrdDestructionError();

        args.Error = MakeError(E_INVALID_STATE, TStringBuilder() <<
            "attempting to clean up unmarked disk");

        return;
    }

    TDiskRegistryDatabase db(tx.DB);
    args.Error = State->DeallocateDisk(db, args.DiskId);

    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "RemoveDisk error: %s. DiskId=%s",
            FormatError(args.Error).c_str(),
            args.DiskId.Quote().c_str());

        return;
    }
}

void TDiskRegistryActor::CompleteRemoveDisk(
    const TActorContext& ctx,
    TTxDiskRegistry::TRemoveDisk& args)
{
    auto response = std::make_unique<TEvDiskRegistry::TEvDeallocateDiskResponse>();
    *response->Record.MutableError() = std::move(args.Error);

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

    SecureErase(ctx);
    NotifyUsers(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage
