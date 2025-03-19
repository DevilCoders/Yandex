#include "tablet_actor.h"

#include "helpers.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TError ValidateRequest(const NProto::TGetNodeAttrRequest& request)
{
    if (request.GetNodeId() == InvalidNodeId) {
        return ErrorInvalidArgument();
    }

    if (!request.GetName().empty()) {
        if (auto error = ValidateNodeName(request.GetName()); HasError(error)) {
            return error;
        }
    }

    return {};
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleGetNodeAttr(
    const TEvService::TEvGetNodeAttrRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(GetNodeAttr, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto nodeId = msg->Record.GetNodeId();
    const auto& name = msg->Record.GetName();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] GetNodeAttr (%lu, %s)",
        TabletID(),
        sessionId.Quote().c_str(),
        nodeId,
        name.Quote().c_str());

    auto error = ValidateRequest(msg->Record);
    if (FAILED(error.GetCode())) {
        auto response = std::make_unique<TEvService::TEvGetNodeAttrResponse>(
            error);

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TGetNodeAttr>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_GetNodeAttr(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TGetNodeAttr& args)
{
    Y_UNUSED(ctx);

    auto* session = FindSession(args.ClientId, args.SessionId);
    if (!session) {
        args.Error = ErrorInvalidSession(args.ClientId, args.SessionId);
        return true;
    }

    args.CommitId = GetReadCommitId(session->GetCheckpointId());
    if (args.CommitId == InvalidCommitId) {
        args.Error = ErrorInvalidCheckpoint(session->GetCheckpointId());
        return true;
    }

    TIndexTabletDatabase db(tx.DB);

    // There could be two cases:
    // * access by parentId/name
    // * access by nodeId

    if (args.Name) {
        // validate parent node exists
        if (!ReadNode(db, args.NodeId, args.CommitId, args.ParentNode)) {
            return false;   // not ready
        }

        if (!args.ParentNode) {
            args.Error = ErrorInvalidParent(args.NodeId);
            return true;
        }

        // TODO: access check
        Y_VERIFY(args.ParentNode);

        // validate target node exists
        TMaybe<TIndexTabletDatabase::TNodeRef> ref;
        if (!ReadNodeRef(db, args.NodeId, args.CommitId, args.Name, ref)) {
            return false;   // not ready
        }

        if (!ref) {
            args.Error = ErrorInvalidTarget(args.NodeId, args.Name);
            return true;
        }

        args.TargetNodeId = ref->ChildNodeId;
    } else {
        args.TargetNodeId = args.NodeId;
    }

    if (!ReadNode(db, args.TargetNodeId, args.CommitId, args.TargetNode)) {
        return false;   // not ready
    }

    if (!args.TargetNode) {
        args.Error = ErrorInvalidTarget(args.TargetNodeId, args.Name);
        return true;
    }

    // TODO: AccessCheck
    Y_VERIFY(args.TargetNode);

    return true;
}

void TIndexTabletActor::ExecuteTx_GetNodeAttr(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TGetNodeAttr& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TIndexTabletActor::CompleteTx_GetNodeAttr(
    const TActorContext& ctx,
    TTxIndexTablet::TGetNodeAttr& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] GetNodeAttr completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvGetNodeAttrResponse>(args.Error);

    if (SUCCEEDED(args.Error.GetCode())) {
        Y_VERIFY(args.TargetNode);
        auto* node = response->Record.MutableNode();
        ConvertNodeFromAttrs(*node, args.TargetNodeId, args.TargetNode->Attrs);
    }

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
