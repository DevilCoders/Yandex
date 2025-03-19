#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TError ValidateRequest(const NProto::TRenameNodeRequest& request)
{
    if (request.GetNodeId() == InvalidNodeId || request.GetNewParentId() == InvalidNodeId) {
        return ErrorInvalidArgument();
    }

    // either part
    if (auto error = ValidateNodeName(request.GetName()); HasError(error)) {
        return error;
    }

    if (auto error = ValidateNodeName(request.GetNewName()); HasError(error)) {
        return error;
    }

    return {};
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleRenameNode(
    const TEvService::TEvRenameNodeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_DUPEVENT_SESSION(RenameNode, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto parentNodeId = msg->Record.GetNodeId();
    const auto& name = msg->Record.GetName();
    const auto newParentNodeId = msg->Record.GetNewParentId();
    const auto& newName = msg->Record.GetNewName();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] RenameNode (%lu, %s) -> (%lu, %s)",
        TabletID(),
        sessionId.Quote().c_str(),
        parentNodeId,
        name.Quote().c_str(),
        newParentNodeId,
        newName.Quote().c_str());

    auto error = ValidateRequest(msg->Record);
    if (FAILED(error.GetCode())) {
        auto response = std::make_unique<TEvService::TEvRenameNodeResponse>(
            error);

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TRenameNode>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_RenameNode(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TRenameNode& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_SESSION(RenameNode, args);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GetCurrentCommitId();

    // validate parent node exists
    if (!ReadNode(db, args.ParentNodeId, args.CommitId, args.ParentNode)) {
        return false;   // not ready
    }

    if (!args.ParentNode) {
        args.Error = ErrorInvalidParent(args.ParentNodeId);
        return true;
    }

    // TODO: AccessCheck
    Y_VERIFY(args.ParentNode);

    // validate old ref exists
    if (!ReadNodeRef(db, args.ParentNodeId, args.CommitId, args.Name, args.ChildRef)) {
        return false;   // not ready
    }

    // read old node
    if (!args.ChildRef) {
        args.Error = ErrorInvalidTarget(args.ParentNodeId);
        return true;
    }

    if (!ReadNode(db, args.ChildRef->ChildNodeId, args.CommitId, args.ChildNode)) {
        return false;
    }

    // TODO: AccessCheck
    Y_VERIFY(args.ChildNode);

    // validate new parent node exists
    if (!ReadNode(db, args.NewParentNodeId, args.CommitId, args.NewParentNode)) {
        return false;   // not ready
    }

    if (!args.NewParentNode) {
        args.Error = ErrorInvalidTarget(args.NewParentNodeId, args.NewName);
        return true;
    }

    // TODO: AccessCheck
    Y_VERIFY(args.NewParentNode);

    // check if new ref exists
    if (!ReadNodeRef(db, args.NewParentNodeId, args.CommitId, args.NewName, args.NewChildRef)) {
        return false;   // not ready
    }

    if (args.NewChildRef) {
        // read new child node to unlink it
        if (!ReadNode(db, args.NewChildRef->ChildNodeId, args.CommitId, args.NewChildNode)) {
            return false;
        }

        // TODO: AccessCheck
        Y_VERIFY(args.NewChildNode);
        // oldpath and newpath are existing hard links to the same file, then rename() does nothing
        if (args.ChildNode->NodeId == args.NewChildNode->NodeId) {
            args.Error = MakeError(S_ALREADY, "is the same file");
            return true;
        }

        // oldpath directory: newpath must either not exist, or it must specify an empty directory.
        if (args.ChildNode->Attrs.GetType() == NProto::E_DIRECTORY_NODE) {
            if (args.NewChildNode->Attrs.GetType() != NProto::E_DIRECTORY_NODE) {
                args.Error = ErrorIsNotDirectory(args.NewChildNode->NodeId);
                return true;
            }
        }

        if (args.NewChildNode->Attrs.GetType() == NProto::E_DIRECTORY_NODE) {
            if (args.ChildNode->Attrs.GetType() != NProto::E_DIRECTORY_NODE) {
                args.Error = ErrorIsDirectory(args.NewChildNode->NodeId);
                return true;
            }

            // 1 entry is enough to prevent rename
            TVector<TIndexTabletDatabase::TNodeRef> refs;
            if (!ReadNodeRefs(db, args.NewChildNode->NodeId, args.CommitId, {}, refs, 1)) {
                return false;
            }

            if (!refs.empty()) {
                args.Error = ErrorIsNotEmpty(args.NewChildNode->NodeId);
                return true;
            }
        }
    }

    return true;
}

void TIndexTabletActor::ExecuteTx_RenameNode(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TRenameNode& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(RenameNode, args);
    if (args.Error.GetCode() == S_ALREADY) {
        return; // nothing to do
    }

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GenerateCommitId();

    if (args.NewChildNode) {
        // remove existing target node
        UnlinkNode(
            db,
            args.NewParentNode->NodeId,
            args.NewName,
            *args.NewChildNode,
            args.NewChildRef->MinCommitId,
            args.CommitId);
    }

    // remove existing source ref
    RemoveNodeRef(
        db,
        args.ParentNodeId,
        args.ChildRef->MinCommitId,
        args.CommitId,
        args.Name,
        args.ChildRef->ChildNodeId);

    // update old parent timestamps
    auto parent = CopyAttrs(args.ParentNode->Attrs, E_CM_CMTIME);
    UpdateNode(
        db,
        args.ParentNode->NodeId,
        args.ParentNode->MinCommitId,
        args.CommitId,
        parent,
        args.ParentNode->Attrs);

    CreateNodeRef(
        db,
        args.NewParentNodeId,
        args.CommitId,
        args.NewName,
        args.ChildRef->ChildNodeId);

    auto newparent = CopyAttrs(args.NewParentNode->Attrs, E_CM_CMTIME);
    UpdateNode(
        db,
        args.NewParentNode->NodeId,
        args.NewParentNode->MinCommitId,
        args.CommitId,
        newparent,
        args.NewParentNode->Attrs);

    auto* session = FindSession(args.SessionId);
    Y_VERIFY(session);

    AddDupCacheEntry(
        db,
        session,
        args.RequestId,
        NProto::TRenameNodeResponse{},
        Config->GetDupCacheEntryCount());
}

void TIndexTabletActor::CompleteTx_RenameNode(
    const TActorContext& ctx,
    TTxIndexTablet::TRenameNode& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] RenameNode completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    if (SUCCEEDED(args.Error.GetCode())) {
        Y_VERIFY(args.ChildRef);

        NProto::TSessionEvent sessionEvent;
        {
            auto* unlinked = sessionEvent.AddNodeUnlinked();
            unlinked->SetParentNodeId(args.ParentNodeId);
            unlinked->SetChildNodeId(args.ChildRef->ChildNodeId);
            unlinked->SetName(args.Name);
        }
        {
            auto* linked = sessionEvent.AddNodeLinked();
            linked->SetParentNodeId(args.NewParentNodeId);
            linked->SetChildNodeId(args.ChildRef->ChildNodeId);
            linked->SetName(args.NewName);
        }
        NotifySessionEvent(ctx, sessionEvent);

        CommitDupCacheEntry(args.SessionId, args.RequestId);
    }

    auto response = std::make_unique<TEvService::TEvRenameNodeResponse>(args.Error);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
