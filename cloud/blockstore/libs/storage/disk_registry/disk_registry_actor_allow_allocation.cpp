#include "disk_registry_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleAllowDiskAllocation(
    const TEvDiskRegistry::TEvAllowDiskAllocationRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(AllowDiskAllocation);

    const auto* msg = ev->Get();
    const bool allow = msg->Record.GetAllow();

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received AllowDiskAllocation request: Allow=%s",
        TabletID(),
        allow ? "true" : "false");

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    ExecuteTx<TAllowDiskAllocation>(
        ctx,
        std::move(requestInfo),
        allow);
}

bool TDiskRegistryActor::PrepareAllowDiskAllocation(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TAllowDiskAllocation& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteAllowDiskAllocation(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TAllowDiskAllocation& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);
    args.Error = State->AllowDiskAllocation(db, args.Allow);
}

void TDiskRegistryActor::CompleteAllowDiskAllocation(
    const TActorContext& ctx,
    TTxDiskRegistry::TAllowDiskAllocation& args)
{
    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "AllowDiskAllocation error: %s",
            FormatError(args.Error).c_str());
    }

    auto response = std::make_unique<TEvDiskRegistry::TEvAllowDiskAllocationResponse>(
        std::move(args.Error));

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
