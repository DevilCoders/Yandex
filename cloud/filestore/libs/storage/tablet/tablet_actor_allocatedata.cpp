#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TError ValidateRequest(
    const NProto::TAllocateDataRequest& request,
    ui32 blockSize)
{
    if (!request.GetHandle()) {
        return ErrorInvalidHandle();
    }

    if (request.GetOffset() > Max<ui64>() - request.GetLength()) {
        return ErrorFileTooBig();
    }

    TByteRange range(request.GetOffset(), request.GetLength(), blockSize);
    if (range.BlockCount() > MaxFileBlocks) {
        return ErrorFileTooBig();
    }

    return {};
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleAllocateData(
    const TEvService::TEvAllocateDataRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(AllocateData, msg->Record);

    auto error = ValidateRequest(msg->Record, GetBlockSize());
    if (FAILED(error.GetCode())) {
        auto response = std::make_unique<TEvService::TEvAllocateDataResponse>(
            error);

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TAllocateData>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_AllocateData(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TAllocateData& args)
{
    Y_UNUSED(ctx);

    auto* session = FindSession(args.ClientId, args.SessionId);
    if (!session) {
        args.Error = ErrorInvalidSession(args.ClientId, args.SessionId);
        return true;
    }

    auto* handle = FindHandle(args.Handle);
    if (!handle || handle->Session != session) {
        args.Error = ErrorInvalidHandle(args.Handle);
        return true;
    }

    if (!HasFlag(handle->GetFlags(), NProto::TCreateHandleRequest::E_WRITE)) {
        args.Error = ErrorInvalidHandle(args.Handle);
        return true;
    }

    args.NodeId = handle->GetNodeId();
    args.CommitId = GetCurrentCommitId();

    TIndexTabletDatabase db(tx.DB);

    if (!ReadNode(db, args.NodeId, args.CommitId, args.Node)) {
        return false;
    }

    if (!args.Node) {
        args.Error = ErrorInvalidTarget(args.NodeId);
        return true;
    }

    if (!HasSpaceLeft(args.Node->Attrs, args.Offset + args.Length)) {
        args.Error = ErrorNoSpaceLeft();
        return true;
    }

    // TODO: AccessCheck
    Y_VERIFY(args.Node);

    return true;
}

void TIndexTabletActor::ExecuteTx_AllocateData(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TAllocateData& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(AllocateData, args);

    const ui64 size = args.Offset + args.Length;
    if (args.Node->Attrs.GetSize() >= size) {
        // nothing todo
        return;
    }

    TIndexTabletDatabase db(tx.DB);
    args.CommitId = GenerateCommitId();

    auto attrs = CopyAttrs(args.Node->Attrs, E_CM_CMTIME);
    attrs.SetSize(size);

    UpdateNode(
        db,
        args.NodeId,
        args.Node->MinCommitId,
        args.CommitId,
        attrs,
        args.Node->Attrs);
}

void TIndexTabletActor::CompleteTx_AllocateData(
    const TActorContext& ctx,
    TTxIndexTablet::TAllocateData& args)
{
    auto response = std::make_unique<TEvService::TEvAllocateDataResponse>(args.Error);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
