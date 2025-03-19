#include "disk_registry_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleSuspendDevice(
    const TEvDiskRegistry::TEvSuspendDeviceRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(SuspendDevice);

    const auto* msg = ev->Get();
    const TString& deviceId = msg->Record.GetDeviceId();

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received SuspendDevice request: DeviceId=%s",
        TabletID(),
        deviceId.c_str());

    if (deviceId.empty()) {
        auto response = std::make_unique<TEvDiskRegistry::TEvSuspendDeviceResponse>(
            MakeError(E_ARGUMENT, "empty device id"));

        NCloud::Reply(ctx, *ev, std::move(response));
        
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TSuspendDevice>(
        ctx,
        std::move(requestInfo),
        deviceId);
}

bool TDiskRegistryActor::PrepareSuspendDevice(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TSuspendDevice& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteSuspendDevice(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TSuspendDevice& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);

    args.Error = State->SuspendDevice(db, args.DeviceId);
}

void TDiskRegistryActor::CompleteSuspendDevice(
    const TActorContext& ctx,
    TTxDiskRegistry::TSuspendDevice& args)
{
    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "SuspendDevice error: %s",
            FormatError(args.Error).c_str());
    }

    auto response = std::make_unique<TEvDiskRegistry::TEvSuspendDeviceResponse>(
        std::move(args.Error));

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
