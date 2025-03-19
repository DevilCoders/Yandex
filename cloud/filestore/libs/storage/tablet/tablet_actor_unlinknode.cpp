#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TError ValidateRequest(const NProto::TUnlinkNodeRequest& request)
{
    if (request.GetNodeId() == InvalidNodeId) {
        return ErrorInvalidArgument();
    }

    if (auto error = ValidateNodeName(request.GetName()); HasError(error)) {
        return error;
    }

    return {};
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleUnlinkNode(
    const TEvService::TEvUnlinkNodeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_DUPEVENT_SESSION(UnlinkNode, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto parentNodeId = msg->Record.GetNodeId();
    const auto& name = msg->Record.GetName();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] UnlinkNode (%lu, %s)",
        TabletID(),
        sessionId.Quote().c_str(),
        parentNodeId,
        name.Quote().c_str());

    auto error = ValidateRequest(msg->Record);
    if (FAILED(error.GetCode())) {
        auto response = std::make_unique<TEvService::TEvUnlinkNodeResponse>(
            error);

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TUnlinkNode>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_UnlinkNode(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TUnlinkNode& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_SESSION(UnlinkNode, args);

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

    // validate target node exists
    if (!ReadNodeRef(db, args.ParentNodeId, args.CommitId, args.Name, args.ChildRef)) {
        return false;   // not ready
    }

    if (!args.ChildRef) {
        args.Error = ErrorInvalidTarget(args.ParentNodeId);
        return true;
    }

    if (!ReadNode(db, args.ChildRef->ChildNodeId, args.CommitId, args.ChildNode)) {
        return false;   // not ready
    }

    // TODO: AccessCheck
    Y_VERIFY(args.ChildNode);
    if (args.ChildNode->Attrs.GetType() == NProto::E_DIRECTORY_NODE) {
        TVector<TIndexTabletDatabase::TNodeRef> refs;
        // 1 entry is enough to prevent deletion
        if (!ReadNodeRefs(db, args.ChildRef->ChildNodeId, args.CommitId, {}, refs, 1)) {
            return false;
        }

        if (!refs.empty()) {
            // cannot unlink non empty directory
            args.Error = ErrorIsNotEmpty(args.ParentNodeId);
            return true;
        }

        if (!args.Request.GetUnlinkDirectory()) {
            // should expliciltly unlink directory node
            args.Error = ErrorIsDirectory(args.ParentNodeId);
            return true;
        }
    } else if (args.Request.GetUnlinkDirectory()) {
        args.Error = ErrorIsNotDirectory(args.ParentNodeId);
        return true;
    }

    return true;
}

void TIndexTabletActor::ExecuteTx_UnlinkNode(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TUnlinkNode& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(UnlinkNode, args);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GenerateCommitId();

    UnlinkNode(
        db,
        args.ParentNodeId,
        args.Name,
        *args.ChildNode,
        args.ChildRef->MinCommitId,
        args.CommitId);

    auto* session = FindSession(args.SessionId);
    Y_VERIFY(session);

    AddDupCacheEntry(
        db,
        session,
        args.RequestId,
        NProto::TUnlinkNodeResponse{},
        Config->GetDupCacheEntryCount());
}

void TIndexTabletActor::CompleteTx_UnlinkNode(
    const TActorContext& ctx,
    TTxIndexTablet::TUnlinkNode& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] UnlinkNode completed (%s)",
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
        NotifySessionEvent(ctx, sessionEvent);

        CommitDupCacheEntry(args.SessionId, args.RequestId);
    }

    auto response = std::make_unique<TEvService::TEvUnlinkNodeResponse>(args.Error);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
