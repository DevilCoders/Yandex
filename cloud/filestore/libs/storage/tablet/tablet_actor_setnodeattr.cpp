#include "tablet_actor.h"

#include "helpers.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TError ValidateRequest(const NProto::TSetNodeAttrRequest& request, ui32 blockSize)
{
    if (request.GetNodeId() == InvalidNodeId || request.GetFlags() == 0) {
        return ErrorInvalidArgument();
    }

    if (HasFlag(request.GetFlags(), NProto::TSetNodeAttrRequest::F_SET_ATTR_SIZE) &&
        request.GetUpdate().GetSize() > 0)
    {
        TByteRange range(0, request.GetUpdate().GetSize(), blockSize);
        if (range.BlockCount() > MaxFileBlocks) {
            return ErrorFileTooBig();
        }
    }

    return {};
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleSetNodeAttr(
    const TEvService::TEvSetNodeAttrRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(SetNodeAttr, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] SetNodeAttr: %s",
        TabletID(),
        sessionId.Quote().c_str(),
        DumpMessage(msg->Record).c_str());

    auto error = ValidateRequest(msg->Record, GetBlockSize());
    if (FAILED(error.GetCode())) {
        auto response = std::make_unique<TEvService::TEvSetNodeAttrResponse>(
            error);

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TSetNodeAttr>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_SetNodeAttr(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TSetNodeAttr& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_SESSION(SetNodeAttr, args);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GetCurrentCommitId();

    // validate target node exists
    if (!ReadNode(db, args.NodeId, args.CommitId, args.Node)) {
        return false;   // not ready
    }

    if (!args.Node) {
        args.Error = ErrorInvalidTarget(args.NodeId);
        return true;
    }

    auto flags = args.Request.GetFlags();
    if (HasFlag(flags, NProto::TSetNodeAttrRequest::F_SET_ATTR_SIZE)) {
        const auto& update = args.Request.GetUpdate();
        if (!HasSpaceLeft(args.Node->Attrs, update.GetSize())) {
            args.Error = ErrorNoSpaceLeft();
            return true;
        }
    }

    // TODO: AccessCheck
    Y_VERIFY(args.Node);

    return true;
}

void TIndexTabletActor::ExecuteTx_SetNodeAttr(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TSetNodeAttr& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(SetNodeAttr, args);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GenerateCommitId();

    auto flags = args.Request.GetFlags();
    const auto& update = args.Request.GetUpdate();

    ECopyAttrsMode mode;
    if (HasFlag(flags, NProto::TSetNodeAttrRequest::F_SET_ATTR_SIZE)) {
        // upon size change e.g. truncate we should also update mtime
        mode = E_CM_CMTIME;
    } else {
        // other attributes trigger only ctime change
        mode = E_CM_CTIME;
    }

    NProto::TNode attrs = CopyAttrs(args.Node->Attrs, mode);
    if (HasFlag(flags, NProto::TSetNodeAttrRequest::F_SET_ATTR_MODE)) {
        attrs.SetMode(update.GetMode());
    }
    if (HasFlag(flags, NProto::TSetNodeAttrRequest::F_SET_ATTR_UID)) {
        attrs.SetUid(update.GetUid());
    }
    if (HasFlag(flags, NProto::TSetNodeAttrRequest::F_SET_ATTR_GID)) {
        attrs.SetGid(update.GetGid());
    }
    if (HasFlag(flags, NProto::TSetNodeAttrRequest::F_SET_ATTR_ATIME)) {
        attrs.SetATime(update.GetATime());
    }
    if (HasFlag(flags, NProto::TSetNodeAttrRequest::F_SET_ATTR_MTIME)) {
        attrs.SetMTime(update.GetMTime());
    }
    if (HasFlag(flags, NProto::TSetNodeAttrRequest::F_SET_ATTR_CTIME)) {
        attrs.SetCTime(update.GetCTime());
    }
    if (HasFlag(flags, NProto::TSetNodeAttrRequest::F_SET_ATTR_SIZE)) {
        Truncate(
            db,
            args.NodeId,
            args.CommitId,
            attrs.GetSize(),
            update.GetSize());

        attrs.SetSize(update.GetSize());
    }

    UpdateNode(
        db,
        args.NodeId,
        args.Node->MinCommitId,
        args.CommitId,
        attrs,
        args.Node->Attrs);

    args.Node->Attrs = std::move(attrs);
}

void TIndexTabletActor::CompleteTx_SetNodeAttr(
    const TActorContext& ctx,
    TTxIndexTablet::TSetNodeAttr& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] SetNodeAttr completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvSetNodeAttrResponse>(args.Error);

    if (SUCCEEDED(args.Error.GetCode())) {
        Y_VERIFY(args.Node);

        auto* node = response->Record.MutableNode();
        ConvertNodeFromAttrs(*node, args.NodeId, args.Node->Attrs);

        NProto::TSessionEvent sessionEvent;
        {
            auto* changed = sessionEvent.AddNodeChanged();
            changed->SetNodeId(args.NodeId);
            changed->SetKind(NProto::TSessionEvent::NODE_ATTR_CHANGED);
        }
        NotifySessionEvent(ctx, sessionEvent);
    }

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
