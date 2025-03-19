#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleListNodeXAttr(
    const TEvService::TEvListNodeXAttrRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(ListNodeXAttr, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto nodeId = msg->Record.GetNodeId();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ListNodeXAttr %lu",
        TabletID(),
        sessionId.Quote().c_str(),
        nodeId);

    if (nodeId == InvalidNodeId) {
        auto response = std::make_unique<TEvService::TEvListNodeXAttrResponse>(
            ErrorInvalidArgument());

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TListNodeXAttr>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_ListNodeXAttr(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TListNodeXAttr& args)
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
    }

    // TODO: AccessCheck
    Y_VERIFY(args.Node);

    if (!ReadNodeAttrs(db, args.NodeId, args.CommitId, args.Attrs)) {
        return false;   // not ready
    }

    return true;
}

void TIndexTabletActor::ExecuteTx_ListNodeXAttr(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TListNodeXAttr& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TIndexTabletActor::CompleteTx_ListNodeXAttr(
    const TActorContext& ctx,
    TTxIndexTablet::TListNodeXAttr& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ListNodeXAttr completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvListNodeXAttrResponse>(args.Error);

    if (SUCCEEDED(args.Error.GetCode())) {
        for (const auto& attr: args.Attrs) {
            response->Record.AddNames(attr.Name);
        }
    }

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
