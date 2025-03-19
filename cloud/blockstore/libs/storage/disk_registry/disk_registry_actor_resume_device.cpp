#include "disk_registry_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleResumeDevice(
    const TEvDiskRegistry::TEvResumeDeviceRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(ResumeDevice);

    const auto* msg = ev->Get();
    const TString& host = msg->Record.GetHost();
    const TString& path = msg->Record.GetPath();

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received ResumeDevice request: Host=%s Path=%s",
        TabletID(),
        host.c_str(),
        path.c_str());

    auto deviceId = State->GetDeviceId(host, path);

    if (deviceId.empty()) {
        auto response = std::make_unique<TEvDiskRegistry::TEvResumeDeviceResponse>(
            MakeError(E_NOT_FOUND, TStringBuilder() <<
                "device not found: " << host << ":" << path));

        NCloud::Reply(ctx, *ev, std::move(response));
        
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TResumeDevice>(
        ctx,
        std::move(requestInfo),
        std::move(deviceId));
}

bool TDiskRegistryActor::PrepareResumeDevice(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TResumeDevice& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteResumeDevice(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TResumeDevice& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);

    args.Error = State->ResumeDevice(db, args.DeviceId);
}

void TDiskRegistryActor::CompleteResumeDevice(
    const TActorContext& ctx,
    TTxDiskRegistry::TResumeDevice& args)
{
    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "ResumeDevice error: %s",
            FormatError(args.Error).c_str());
    }

    auto response = std::make_unique<TEvDiskRegistry::TEvResumeDeviceResponse>(
        std::move(args.Error));

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

    SecureErase(ctx);

    StartMigration(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage
