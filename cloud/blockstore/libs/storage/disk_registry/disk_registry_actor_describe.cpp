#include "disk_registry_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleDescribeDisk(
    const TEvDiskRegistry::TEvDescribeDiskRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(DescribeDisk);

    const auto* msg = ev->Get();
    const TString& diskId = msg->Record.GetDiskId();

    LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received DescribeDisk request: DiskId=%s",
        TabletID(),
        diskId.c_str());

    if (!diskId) {
        auto response = std::make_unique<TEvDiskRegistry::TEvDescribeDiskResponse>(
            MakeError(E_ARGUMENT, "empty disk id"));
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    TDiskInfo diskInfo;

    auto error = State->GetDiskInfo(diskId, diskInfo);

    if (HasError(error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "GetDiskInfo error: %s",
            FormatError(error).c_str());
    }

    auto response = std::make_unique<TEvDiskRegistry::TEvDescribeDiskResponse>(
        std::move(error)
    );

    response->Record.SetState(diskInfo.State);
    response->Record.SetStateTs(diskInfo.StateTs.MicroSeconds());

    auto onDevice = [&] (NProto::TDeviceConfig& d, ui32 blockSize) {
        if (ToLogicalBlocks(d, blockSize)) {
            return;
        }

        TStringBuilder error;
        error << "HandleDescribeDisk: ToLogicalBlocks failed, device: "
            << d.GetDeviceUUID().Quote().c_str();
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY, error);
    };

    for (auto& device: diskInfo.Devices) {
        onDevice(device, diskInfo.LogicalBlockSize);
        *response->Record.AddDevices() = std::move(device);
    }

    for (auto& migration: diskInfo.Migrations) {
        onDevice(
            *migration.MutableTargetDevice(),
            diskInfo.LogicalBlockSize
        );
        *response->Record.AddMigrations() = std::move(migration);
    }

    for (auto& replica: diskInfo.Replicas) {
        auto* r = response->Record.AddReplicas();
        for (auto& device: replica) {
            onDevice(device, diskInfo.LogicalBlockSize);
            *r->AddDevices() = std::move(device);
        }
    }

    NCloud::Reply(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
