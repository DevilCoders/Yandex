#include "disk_registry_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleSetUserId(
    const TEvDiskRegistry::TEvSetUserIdRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(SetUserId);

    const auto* msg = ev->Get();
    const auto& diskId = msg->Record.GetDiskId();
    const auto& userId = msg->Record.GetUserId();

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received SetUserId request: DiskId=%s UserId=%s",
        TabletID(),
        diskId.c_str(),
        userId.c_str());

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TSetUserId>(
        ctx,
        std::move(requestInfo),
        diskId,
        userId);
}

bool TDiskRegistryActor::PrepareSetUserId(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TSetUserId& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteSetUserId(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TSetUserId& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);
    args.Error = State->SetUserId(db, args.DiskId, args.UserId);
}

void TDiskRegistryActor::CompleteSetUserId(
    const TActorContext& ctx,
    TTxDiskRegistry::TSetUserId& args)
{
    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "SetUserId error: %s",
            FormatError(args.Error).c_str());
    }

    auto response = std::make_unique<TEvDiskRegistry::TEvSetUserIdResponse>(
        std::move(args.Error));

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
