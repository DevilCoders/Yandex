#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleReleaseLock(
    const TEvService::TEvReleaseLockRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(ReleaseLock, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    ui64 handle = msg->Record.GetHandle();
    TByteRange byteRange(
        msg->Record.GetOffset(),
        msg->Record.GetLength(),
        GetBlockSize()
    );

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ReleaseLock @%lu %s",
        TabletID(),
        sessionId.Quote().c_str(),
        handle,
        byteRange.Describe().c_str());

    auto error = ValidateRangeRequest(byteRange);
    if (FAILED(error.GetCode())) {
        auto response = std::make_unique<TEvService::TEvReleaseLockResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TReleaseLock>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_ReleaseLock(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TReleaseLock& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);

    FILESTORE_VALIDATE_TX_SESSION(ReleaseLock, args);

    return true;
}

void TIndexTabletActor::ExecuteTx_ReleaseLock(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TReleaseLock& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(ReleaseLock, args);

    auto* session = FindSession(args.ClientId, args.SessionId);
    Y_VERIFY(session);

    auto* handle = FindHandle(args.Request.GetHandle());
    if (!handle || handle->GetSessionId() != session->GetSessionId()) {
        args.Error = MakeError(E_FS_BADHANDLE, "invalid handle");
        return;
    }

    TLockRange range = {
        .NodeId = handle->GetNodeId(),
        .OwnerId = args.Request.GetOwner(),
        .Offset = args.Request.GetOffset(),
        .Length = args.Request.GetLength()
    };

    TIndexTabletDatabase db(tx.DB);
    ReleaseLock(db, session, range);
}

void TIndexTabletActor::CompleteTx_ReleaseLock(
    const TActorContext& ctx,
    TTxIndexTablet::TReleaseLock& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ReleaseLock completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvReleaseLockResponse>(args.Error);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
