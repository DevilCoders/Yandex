#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleReadLink(
    const TEvService::TEvReadLinkRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(ReadLink, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto nodeId = msg->Record.GetNodeId();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ReadLink @%lu",
        TabletID(),
        sessionId.Quote().c_str(),
        nodeId);

    if (nodeId == InvalidNodeId) {
        auto response = std::make_unique<TEvService::TEvReadLinkResponse>(
            ErrorInvalidArgument());

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TReadLink>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_ReadLink(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TReadLink& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_SESSION(ReadLink, args);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GetCurrentCommitId();

    // validate parent node exists
    if (!ReadNode(db, args.NodeId, args.CommitId, args.Node)) {
        return false;   // not ready
    }

    if (!args.Node || args.Node->Attrs.GetType() != NProto::E_LINK_NODE) {
        args.Error = ErrorInvalidTarget(args.NodeId);
        return true;
    }

    // TODO: AccessCheck
    Y_VERIFY(args.Node);

    return true;
}

void TIndexTabletActor::ExecuteTx_ReadLink(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TReadLink& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TIndexTabletActor::CompleteTx_ReadLink(
    const TActorContext& ctx,
    TTxIndexTablet::TReadLink& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ReadLink completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvReadLinkResponse>(args.Error);
    if (SUCCEEDED(args.Error.GetCode())) {
        response->Record.SetSymLink(args.Node->Attrs.GetSymLink());
    }

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
