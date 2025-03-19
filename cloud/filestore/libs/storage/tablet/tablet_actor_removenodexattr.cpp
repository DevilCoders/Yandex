#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleRemoveNodeXAttr(
    const TEvService::TEvRemoveNodeXAttrRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(RemoveNodeXAttr, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto nodeId = msg->Record.GetNodeId();
    const auto& name = msg->Record.GetName();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] RemoveNodeXAttr %lu @ %s",
        TabletID(),
        sessionId.Quote().c_str(),
        nodeId,
        name.Quote().c_str());

    if (nodeId == InvalidNodeId) {
        auto response = std::make_unique<TEvService::TEvRemoveNodeXAttrResponse>(
            ErrorInvalidArgument());
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    if (auto error = ValidateXAttrName(name); HasError(error)) {
        auto response = std::make_unique<TEvService::TEvRemoveNodeXAttrResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TRemoveNodeXAttr>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_RemoveNodeXAttr(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TRemoveNodeXAttr& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_SESSION(RemoveNodeXAttr, args);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GetCurrentCommitId();

    // parse path
    if (!ReadNode(db, args.NodeId, args.CommitId, args.Node)) {
        return false;   // not ready
    }

    if (!args.Node) {
        args.Error = ErrorInvalidTarget(args.NodeId);
        return true;
    }

    // TODO: AccessCheck
    Y_VERIFY(args.Node);

    if (!ReadNodeAttr(db, args.NodeId, args.CommitId, args.Name, args.Attr)) {
        return false;   // not ready
    }

    return true;
}

void TIndexTabletActor::ExecuteTx_RemoveNodeXAttr(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TRemoveNodeXAttr& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(RemoveNodeXAttr, args);

    if (!args.Attr) {
        // nothing to do
        return;
    }

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GenerateCommitId();

    RemoveNodeAttr(
        db,
        args.NodeId,
        args.Attr->MinCommitId,
        args.CommitId,
        *args.Attr);
}

void TIndexTabletActor::CompleteTx_RemoveNodeXAttr(
    const TActorContext& ctx,
    TTxIndexTablet::TRemoveNodeXAttr& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] RemoveNodeXAttr completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvRemoveNodeXAttrResponse>(args.Error);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
