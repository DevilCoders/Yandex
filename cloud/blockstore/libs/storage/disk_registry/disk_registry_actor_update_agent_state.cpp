#include "disk_registry_actor.h"
#include "disk_registry_database.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleChangeAgentState(
    const TEvDiskRegistry::TEvChangeAgentStateRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(ChangeAgentState);

    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received ChangeAgentState request: AgentId=%s, State=%u",
        TabletID(),
        msg->Record.GetAgentId().c_str(),
        static_cast<ui32>(msg->Record.GetAgentState()));

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    ExecuteTx<TUpdateAgentState>(
        ctx,
        std::move(requestInfo),
        std::move(*msg->Record.MutableAgentId()),
        msg->Record.GetAgentState(),
        ctx.Now(),
        msg->Record.GetReason());
}

////////////////////////////////////////////////////////////////////////////////

bool TDiskRegistryActor::PrepareUpdateAgentState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TUpdateAgentState& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteUpdateAgentState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TUpdateAgentState& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);

    args.Error = State->UpdateAgentState(
        db,
        args.AgentId,
        args.State,
        args.StateTs,
        args.Reason,
        args.AffectedDisks);
}

void TDiskRegistryActor::CompleteUpdateAgentState(
    const TActorContext& ctx,
    TTxDiskRegistry::TUpdateAgentState& args)
{
    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "UpdateAgentState error: %s",
            FormatError(args.Error).c_str());
    }

    NotifyDisks(ctx);
    NotifyUsers(ctx);
    PublishDiskStates(ctx);
    SecureErase(ctx);
    StartMigration(ctx);

    auto response = std::make_unique<TEvDiskRegistry::TEvChangeAgentStateResponse>(
        std::move(args.Error));
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
