#include "tablet_actor.h"

#include "helpers.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TError ValidateRequest(const NProto::TCreateNodeRequest& request)
{
    if (request.GetNodeId() == InvalidNodeId ||
        (request.HasLink() && request.GetLink().GetTargetNode() == InvalidNodeId) ||
        (request.HasSymLink() && request.GetSymLink().GetTargetPath().empty()))
    {
        return ErrorInvalidArgument();
    }

    if (auto error = ValidateNodeName(request.GetName()); HasError(error)) {
        return error;
    }

    if (request.HasSymLink()) {
        const auto& path = request.GetSymLink().GetTargetPath();
        if (path.size() > MaxSymlink) {
            return ErrorNameTooLong(path);
        }
    }

    return {};
}

void InitAttrs(NProto::TNode& attrs, const NProto::TCreateNodeRequest& request)
{
    if (request.HasDirectory()) {
        const auto& dir = request.GetDirectory();
        attrs = CreateDirectoryAttrs(dir.GetMode(), request.GetUid(), request.GetGid());
    } else if (request.HasFile()) {
        const auto& file = request.GetFile();
        attrs = CreateRegularAttrs(file.GetMode(), request.GetUid(), request.GetGid());
    } else if (request.HasSymLink()) {
        const auto& link = request.GetSymLink();
        attrs = CreateLinkAttrs(link.GetTargetPath(), request.GetUid(), request.GetGid());
    } else if (request.HasSocket()) {
        const auto& sock = request.GetSocket();
        attrs = CreateSocketAttrs(sock.GetMode(), request.GetUid(), request.GetGid());
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleCreateNode(
    const TEvService::TEvCreateNodeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_DUPEVENT_SESSION(CreateNode, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto parentNodeId = msg->Record.GetNodeId();
    const auto& name = msg->Record.GetName();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] CreateNode (%lu, %s)",
        TabletID(),
        sessionId.Quote().c_str(),
        parentNodeId,
        name.Quote().c_str());

    NProto::TError error = ValidateRequest(msg->Record);
    if (FAILED(error.GetCode())) {
        auto response = std::make_unique<TEvService::TEvCreateNodeResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    ui64 targetNodeId = InvalidNodeId;
    if (msg->Record.HasLink()) {
        targetNodeId = msg->Record.GetLink().GetTargetNode();
    }

    NProto::TNode attrs;
    InitAttrs(attrs, msg->Record);

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TCreateNode>(
        ctx,
        std::move(requestInfo),
        msg->Record,
        parentNodeId,
        targetNodeId,
        name,
        attrs);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_CreateNode(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCreateNode& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_SESSION(CreateNode, args);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GetCurrentCommitId();

    // validate there are enough free inodes
    if (GetUsedNodesCount() >= GetNodesCount()) {
        args.Error = ErrorNoSpaceLeft();
        return true;
    }

    // validate parent node exists
    if (!ReadNode(db, args.ParentNodeId, args.CommitId, args.ParentNode)) {
        return false;   // not ready
    }

    if (!args.ParentNode) {
        args.Error = ErrorInvalidParent(args.ParentNodeId);
        return true;
    }

    if (args.ParentNode->Attrs.GetType() != NProto::E_DIRECTORY_NODE) {
        args.Error = ErrorIsNotDirectory(args.ParentNodeId);
        return true;
    }

    // TODO: AccessCheck
    Y_VERIFY(args.ParentNode);

    // validate target node doesn't exist
    TMaybe<TIndexTabletDatabase::TNodeRef> childRef;
    if (!ReadNodeRef(db, args.ParentNodeId, args.CommitId, args.Name, childRef)) {
        return false;   // not ready
    }

    if (childRef) {
        // mknod, mkdir, link nor symlink does not overwrite existing files
        args.Error = ErrorAlreadyExists(args.Name);
        return true;
    }

    Y_VERIFY(!args.ChildNode);
    if (args.TargetNodeId != InvalidNodeId) {
        // hard link: validate link target
        args.ChildNodeId = args.TargetNodeId;
        if (!ReadNode(db, args.ChildNodeId, args.CommitId, args.ChildNode)) {
            return false;   // not ready
        }

        if (!args.ChildNode){
            // should exist
            args.Error = ErrorInvalidTarget(args.ChildNodeId);
            return true;
        } else if (args.ChildNode->Attrs.GetType() == NProto::E_DIRECTORY_NODE) {
            // should not be a directory
            args.Error = ErrorIsDirectory(args.ChildNodeId);
            return true;
        } else if (args.ChildNode->Attrs.GetLinks() + 1 > MaxLink) {
            // should not have too much links
            args.Error = ErrorMaxLink(args.ChildNodeId);
            return true;
        }

        // TODO: AccessCheck
        Y_VERIFY(args.ChildNode);
    }

    return true;
}

void TIndexTabletActor::ExecuteTx_CreateNode(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCreateNode& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(CreateNode, args);

    auto* session = FindSession(args.SessionId);
    Y_VERIFY(session);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GenerateCommitId();

    if (!args.ChildNode) {
        // new node
        args.ChildNodeId = CreateNode(
            db,
            args.CommitId,
            args.Attrs);

        args.ChildNode = TIndexTabletDatabase::TNode {
            args.ChildNodeId,
            args.Attrs,
            args.CommitId,
            InvalidCommitId
        };
    } else {
        // hard link
        auto attrs = CopyAttrs(args.ChildNode->Attrs, E_CM_CMTIME | E_CM_REF);
        UpdateNode(
            db,
            args.ChildNodeId,
            args.ChildNode->MinCommitId,
            args.CommitId,
            attrs,
            args.ChildNode->Attrs);

        args.ChildNode->Attrs = std::move(attrs);
    }

    // update parents cmtime
    auto parent = CopyAttrs(args.ParentNode->Attrs, E_CM_CMTIME);
    UpdateNode(
        db,
        args.ParentNode->NodeId,
        args.ParentNode->MinCommitId,
        args.CommitId,
        parent,
        args.ParentNode->Attrs);

    Y_VERIFY(args.ChildNodeId != InvalidNodeId);
    CreateNodeRef(
        db,
        args.ParentNodeId,
        args.CommitId,
        args.Name,
        args.ChildNodeId);

    ConvertNodeFromAttrs(*args.Response.MutableNode(), args.ChildNodeId, args.ChildNode->Attrs);

    AddDupCacheEntry(
        db,
        session,
        args.RequestId,
        args.Response,
        Config->GetDupCacheEntryCount());
}

void TIndexTabletActor::CompleteTx_CreateNode(
    const TActorContext& ctx,
    TTxIndexTablet::TCreateNode& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] CreateNode completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvCreateNodeResponse>(args.Error);

    if (SUCCEEDED(args.Error.GetCode())) {
        CommitDupCacheEntry(args.SessionId, args.RequestId);

        Y_VERIFY(args.ChildNode);
        response->Record = std::move(args.Response);

        NProto::TSessionEvent sessionEvent;
        {
            auto* linked = sessionEvent.AddNodeLinked();
            linked->SetParentNodeId(args.ParentNodeId);
            linked->SetChildNodeId(args.ChildNodeId);
            linked->SetName(args.Name);
        }

        NotifySessionEvent(ctx, sessionEvent);
    }

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
