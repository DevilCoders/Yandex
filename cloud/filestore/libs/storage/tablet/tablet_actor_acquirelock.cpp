#include "tablet_actor.h"

#include "helpers.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleAcquireLock(
    const TEvService::TEvAcquireLockRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(AcquireLock, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    ui64 handle = msg->Record.GetHandle();
    TByteRange byteRange(
        msg->Record.GetOffset(),
        msg->Record.GetLength(),
        GetBlockSize()
    );

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] AcquireLock @%lu %s",
        TabletID(),
        sessionId.Quote().c_str(),
        handle,
        byteRange.Describe().c_str());

    auto error = ValidateRangeRequest(byteRange);
    if (FAILED(error.GetCode())) {
        auto response = std::make_unique<TEvService::TEvAcquireLockResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TAcquireLock>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_AcquireLock(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TAcquireLock& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);

    FILESTORE_VALIDATE_TX_SESSION(AcquireLock, args);

    return true;
}

void TIndexTabletActor::ExecuteTx_AcquireLock(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TAcquireLock& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(AcquireLock, args);

    auto* session = FindSession(args.ClientId, args.SessionId);
    Y_VERIFY(session);

    auto* handle = FindHandle(args.Request.GetHandle());
    if (!handle || handle->GetSessionId() != session->GetSessionId()) {
        args.Error = ErrorInvalidHandle();
        return;
    }

    ELockMode mode = GetLockMode(args.Request.GetLockType());
    // FIXME: NBS-2933 validate handle mode for fcntl locks

    TLockRange range = {
        .NodeId = handle->GetNodeId(),
        .OwnerId = args.Request.GetOwner(),
        .Offset = args.Request.GetOffset(),
        .Length = args.Request.GetLength()
    };

    if (!TestLock(session, range, mode, nullptr)) {
        args.Error = ErrorIncompatibleLocks();
        return;
    }

    TIndexTabletDatabase db(tx.DB);

    // TODO: access check
    AcquireLock(db, session, handle->GetHandle(), range, mode);
}

void TIndexTabletActor::CompleteTx_AcquireLock(
    const TActorContext& ctx,
    TTxIndexTablet::TAcquireLock& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] AcquireLock completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvAcquireLockResponse>(args.Error);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
