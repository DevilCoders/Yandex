#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleDestroyHandle(
    const TEvService::TEvDestroyHandleRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(DestroyHandle, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    ui64 h = msg->Record.GetHandle();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] DestroyHandle @%lu",
        TabletID(),
        sessionId.Quote().c_str(),
        h);

    auto* handle = FindHandle(h);
    if (!handle || handle->GetSessionId() != sessionId) {
        auto response = std::make_unique<TEvService::TEvDestroyHandleResponse>(
            MakeError(S_FALSE, "Invalid handle"));

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TDestroyHandle>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_DestroyHandle(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TDestroyHandle& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_SESSION(DestroyHandle, args);
    auto* handle = FindHandle(args.Request.GetHandle());
    Y_VERIFY(handle);

    auto commitId = GetCurrentCommitId();

    TIndexTabletDatabase db(tx.DB);
    if (!ReadNode(db, handle->GetNodeId(), commitId, args.Node)) {
        return false;
    }

    Y_VERIFY(args.Node);

    return true;
}

void TIndexTabletActor::ExecuteTx_DestroyHandle(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TDestroyHandle& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(DestroyHandle, args);

    auto* handle = FindHandle(args.Request.GetHandle());
    Y_VERIFY(handle);

    TIndexTabletDatabase db(tx.DB);
    DestroyHandle(db, handle);

    auto commitId = GenerateCommitId();
    if (args.Node->Attrs.GetLinks() == 0 &&
        !HasOpenHandles(args.Node->NodeId))
    {
        RemoveNode(
            db,
            *args.Node,
            args.Node->MinCommitId,
            commitId);
    }
}

void TIndexTabletActor::CompleteTx_DestroyHandle(
    const TActorContext& ctx,
    TTxIndexTablet::TDestroyHandle& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] DestroyHandle completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvDestroyHandleResponse>(args.Error);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
