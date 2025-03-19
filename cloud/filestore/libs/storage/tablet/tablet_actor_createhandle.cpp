#include "tablet_actor.h"

#include "helpers.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TError ValidateRequest(const NProto::TCreateHandleRequest& request)
{
    if (request.GetNodeId() == InvalidNodeId) {
        return ErrorInvalidArgument();
    }

    if (!request.GetName().empty()) {
        if (auto error = ValidateNodeName(request.GetName()); HasError(error)) {
            return error;
        }
    }

    if (HasFlag(request.GetFlags(), NProto::TCreateHandleRequest::E_TRUNCATE) &&
        !HasFlag(request.GetFlags(), NProto::TCreateHandleRequest::E_WRITE))
    {
        // POSIX: The result of using O_TRUNC without either O_RDWR o O_WRONLY is undefined
        return ErrorInvalidArgument();
    }

    return {};
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleCreateHandle(
    const TEvService::TEvCreateHandleRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_DUPEVENT_SESSION(CreateHandle, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto nodeId = msg->Record.GetNodeId();
    const auto& name = msg->Record.GetName();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] CreateHandle (%lu, %s)",
        TabletID(),
        sessionId.Quote().c_str(),
        nodeId,
        name.Quote().c_str());

    auto error = ValidateRequest(msg->Record);
    if (FAILED(error.GetCode())) {
        auto response = std::make_unique<TEvService::TEvCreateHandleResponse>(
            error);

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TCreateHandle>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_CreateHandle(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCreateHandle& args)
{
    Y_UNUSED(ctx);

    auto* session = FindSession(args.ClientId, args.SessionId);
    if (!session) {
        args.Error = ErrorInvalidSession(args.ClientId, args.SessionId);
        return true;
    }

    args.ReadCommitId = GetReadCommitId(session->GetCheckpointId());
    if (args.ReadCommitId == InvalidCommitId) {
        args.Error = ErrorInvalidCheckpoint(session->GetCheckpointId());
        return true;
    }

    TIndexTabletDatabase db(tx.DB);

    // There could be two cases:
    // * access by parentId/name
    // * access by nodeId
    if (args.Name) {
        // check that parent exists and is the directory;
        // TODO: what if symlink?
        if (!ReadNode(db, args.NodeId, args.ReadCommitId, args.ParentNode)) {
            return false;
        }

        if (!args.ParentNode || args.ParentNode->Attrs.GetType() != NProto::E_DIRECTORY_NODE) {
            args.Error = ErrorIsNotDirectory(args.NodeId);
            return true;
        }

        // check whether child node exists
        TMaybe<TIndexTabletDatabase::TNodeRef> ref;
        if (!ReadNodeRef(db, args.NodeId, args.ReadCommitId, args.Name, ref)) {
            return false;   // not ready
        }

        if (!ref) {
            // if not check whether we should create one
            if (!HasFlag(args.Flags, NProto::TCreateHandleRequest::E_CREATE)) {
                args.Error = ErrorInvalidTarget(args.NodeId, args.Name);
                return true;
            }

            // validate there are enough free inodes
            if (GetUsedNodesCount() >= GetNodesCount()) {
                args.Error = ErrorNoSpaceLeft();
                return true;
            }
        } else {
            // if yes check whether O_EXCL was specified, assume O_CREAT is also specified
            if (HasFlag(args.Flags, NProto::TCreateHandleRequest::E_EXCLUSIVE)) {
                args.Error = ErrorAlreadyExists(args.Name);
                return true;
            }

            args.TargetNodeId = ref->ChildNodeId;
        }
    } else {
        args.TargetNodeId = args.NodeId;
    }

    if (args.TargetNodeId != InvalidNodeId) {
        if (!ReadNode(db, args.TargetNodeId, args.ReadCommitId, args.TargetNode)) {
            return false;   // not ready
        }

        if (!args.TargetNode) {
            args.Error = ErrorInvalidTarget(args.TargetNodeId);
            return true;
        }

        // TODO: support for O_DIRECTORY
        if (args.TargetNode->Attrs.GetType() != NProto::E_REGULAR_NODE) {
            args.Error = ErrorIsDirectory(args.TargetNodeId);
            return true;
        }

        // TODO: AccessCheck
        Y_VERIFY(args.TargetNode);
    }

    return true;
}

void TIndexTabletActor::ExecuteTx_CreateHandle(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCreateHandle& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(CreateHandle, args);

    auto* session = FindSession(args.SessionId);
    Y_VERIFY(session);

    // TODO: check if session is read only
    args.WriteCommitId = GenerateCommitId();
    TIndexTabletDatabase db(tx.DB);

    if (args.TargetNodeId == InvalidNodeId) {
        Y_VERIFY(!args.TargetNode);
        Y_VERIFY(args.ParentNode);

        NProto::TNode attrs = CreateRegularAttrs(args.Mode, args.Uid, args.Gid);
        args.TargetNodeId = CreateNode(
            db,
            args.WriteCommitId,
            attrs);

        args.TargetNode = TIndexTabletDatabase::TNode {
            args.TargetNodeId,
            attrs,
            args.WriteCommitId,
            InvalidCommitId
        };

        // TODO: support for O_TMPFILE
        CreateNodeRef(
            db,
            args.NodeId,
            args.WriteCommitId,
            args.Name,
            args.TargetNodeId);

        // update parent cmtime as we created a new entry
        auto parent = CopyAttrs(args.ParentNode->Attrs, E_CM_CMTIME);
        UpdateNode(
            db,
            args.ParentNode->NodeId,
            args.ParentNode->MinCommitId,
            args.WriteCommitId,
            parent,
            args.ParentNode->Attrs);

    } else if (HasFlag(args.Flags, NProto::TCreateHandleRequest::E_TRUNCATE)) {
        Truncate(
            db,
            args.TargetNodeId,
            args.WriteCommitId,
            args.TargetNode->Attrs.GetSize(),
            0);

        auto attrs = CopyAttrs(args.TargetNode->Attrs, E_CM_CMTIME);
        attrs.SetSize(0);

        UpdateNode(
            db,
            args.TargetNodeId,
            args.TargetNode->MinCommitId,
            args.WriteCommitId,
            attrs,
            args.TargetNode->Attrs);
    }

    auto* handle = CreateHandle(
        db,
        session,
        args.TargetNodeId,
        session->GetCheckpointId() ? args.ReadCommitId : InvalidCommitId,
        args.Flags);

    Y_VERIFY(handle);
    args.Response.SetHandle(handle->GetHandle());
    auto* node = args.Response.MutableNodeAttr();
    ConvertNodeFromAttrs(*node, args.TargetNodeId, args.TargetNode->Attrs);

    AddDupCacheEntry(
        db,
        session,
        args.RequestId,
        args.Response,
        Config->GetDupCacheEntryCount());
}

void TIndexTabletActor::CompleteTx_CreateHandle(
    const TActorContext& ctx,
    TTxIndexTablet::TCreateHandle& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] CreateHandle completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvCreateHandleResponse>(args.Error);
    if (SUCCEEDED(args.Error.GetCode())) {
        CommitDupCacheEntry(args.SessionId, args.RequestId);
        response->Record = std::move(args.Response);
    }

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
