#include "disk_registry_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleCreateDiskFromDevices(
    const TEvDiskRegistry::TEvCreateDiskFromDevicesRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(CreateDiskFromDevices);

    const auto& record = ev->Get()->Record;

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received CreateDiskFromDevices %s, %d, [ %s] %s",
        TabletID(),
        record.GetDiskId().c_str(),
        record.GetBlockSize(),
        [&] {
            TStringStream out;
            for (auto& d: record.GetDevices()) {
                out << "(" << d.GetAgentId() << " " << d.GetDeviceName() << ") ";
            }
            return out.Str();
        }().c_str(),
        record.GetForce() ? "force" : "");

    ExecuteTx<TCreateDiskFromDevices>(
        ctx,
        CreateRequestInfo<TEvDiskRegistry::TCreateDiskFromDevicesMethod>(
            ev->Sender,
            ev->Cookie,
            ev->Get()->CallContext,
            std::move(ev->TraceId)
        ),
        record.GetForce(),
        record.GetDiskId(),
        record.GetBlockSize(),
        TVector<NProto::TDeviceConfig> (
            record.GetDevices().begin(),
            record.GetDevices().end())
        );
}

bool TDiskRegistryActor::PrepareCreateDiskFromDevices(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TCreateDiskFromDevices& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteCreateDiskFromDevices(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TCreateDiskFromDevices& args)
{
    TDiskRegistryDatabase db(tx.DB);

    args.Error = State->CreateDiskFromDevices(
        db,
        args.Force,
        args.DiskId,
        args.BlockSize,
        args.Devices);

    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "CreateDiskFromDevices %s execution failed: %s",
            args.ToString().c_str(),
            FormatError(args.Error).c_str());
        return;
    }

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "CreateDiskFromDevices %s execution succeeded",
        args.ToString().c_str());
}

void TDiskRegistryActor::CompleteCreateDiskFromDevices(
    const TActorContext& ctx,
    TTxDiskRegistry::TCreateDiskFromDevices& args)
{
    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "CreateDiskFromDevices %s complete",
        args.ToString().c_str());

    NotifyDisks(ctx);

    auto response =
        std::make_unique<TEvDiskRegistry::TEvCreateDiskFromDevicesResponse>(
            std::move(args.Error));

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
