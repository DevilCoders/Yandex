#include "tablet_actor.h"

#include "helpers.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

void AddNode(
    NProto::TListNodesResponse& record,
    TString name,
    ui64 id,
    const NProto::TNode& attrs)
{
    record.AddNames(std::move(name));
    ConvertNodeFromAttrs(*record.AddNodes(), id, attrs);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleListNodes(
    const TEvService::TEvListNodesRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(ListNodes, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto nodeId = msg->Record.GetNodeId();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ListNodes %lu",
        TabletID(),
        sessionId.Quote().c_str(),
        nodeId);

    if (nodeId == InvalidNodeId) {
        auto response = std::make_unique<TEvService::TEvListNodesResponse>(
            ErrorInvalidArgument());

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    auto maxBytes = msg->Record.GetMaxBytes();
    if (!maxBytes) {
        maxBytes = Config->GetMaxResponseBytes();
    }

    ExecuteTx<TListNodes>(
        ctx,
        std::move(requestInfo),
        msg->Record,
        maxBytes);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_ListNodes(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TListNodes& args)
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

    // validate target node exists
    if (!ReadNode(db, args.NodeId, args.CommitId, args.Node)) {
        return false;   // not ready
    }

    if (!args.Node) {
        args.Error = ErrorInvalidTarget(args.NodeId);
        return true;
    } else if (args.Node->Attrs.GetType() != NProto::E_DIRECTORY_NODE) {
        args.Error = ErrorIsNotDirectory(args.NodeId);
        return true;
    }

    // TODO: AccessCheck
    Y_VERIFY(args.Node);

    // list nodes
    if (!ReadNodeRefs(
        db,
        args.NodeId,
        args.CommitId,
        args.Cookie,
        args.ChildRefs,
        args.MaxBytes,
        &args.Next))
    {
        return false;   // not ready
    }

    args.ChildNodes.reserve(args.ChildRefs.size());
    for (const auto& ref: args.ChildRefs) {
        TMaybe<TIndexTabletDatabase::TNode> childNode;
        if (!ReadNode(db, ref.ChildNodeId, args.CommitId, childNode)) {
            return false;   // not ready
        }

        // TODO: AccessCheck
        Y_VERIFY(childNode);

        args.ChildNodes.emplace_back(std::move(childNode.GetRef()));
    }

    return true;
}

void TIndexTabletActor::ExecuteTx_ListNodes(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TListNodes& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TIndexTabletActor::CompleteTx_ListNodes(
    const TActorContext& ctx,
    TTxIndexTablet::TListNodes& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ListNodes completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvListNodesResponse>(args.Error);

    if (SUCCEEDED(args.Error.GetCode())) {
        auto& record = response->Record;
        record.MutableNames()->Reserve(args.ChildRefs.size());
        record.MutableNodes()->Reserve(args.ChildRefs.size());

        for (size_t i = 0; i < args.ChildRefs.size(); ++i) {
            const auto& ref = args.ChildRefs[i];
            AddNode(record, ref.Name, ref.ChildNodeId, args.ChildNodes[i].Attrs);
        }

        if (args.Next) {
            record.SetCookie(args.Next);
        }
    }

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
