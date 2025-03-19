#include "tablet_actor.h"

#include <cloud/storage/core/libs/common/helpers.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleSetNodeXAttr(
    const TEvService::TEvSetNodeXAttrRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(SetNodeXAttr, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto nodeId = msg->Record.GetNodeId();
    const auto& name = msg->Record.GetName();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] SetNodeXAttr %lu @ %s",
        TabletID(),
        sessionId.Quote().c_str(),
        nodeId,
        name.Quote().c_str());

    if (nodeId == InvalidNodeId) {
        auto response = std::make_unique<TEvService::TEvSetNodeXAttrResponse>(
            ErrorInvalidArgument());

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    if (auto error = ValidateXAttrName(name); HasError(error)) {
        auto response = std::make_unique<TEvService::TEvSetNodeXAttrResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    if (auto error = ValidateXAttrValue(name, msg->Record.GetValue()); HasError(error)) {
        auto response = std::make_unique<TEvService::TEvSetNodeXAttrResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TSetNodeXAttr>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_SetNodeXAttr(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TSetNodeXAttr& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_SESSION(SetNodeXAttr, args);

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

    // TODO: AccessCheck
    Y_VERIFY(args.Node);

    // fetch previous version
    if (!ReadNodeAttr(db, args.NodeId, args.CommitId, args.Name, args.Attr)) {
        return false;   // not ready
    }

    const auto flags = args.Request.GetFlags();
    if (args.Attr && HasProtoFlag(flags, NProto::TSetNodeXAttrRequest::F_CREATE)) {
        args.Error = ErrorAttributeAlreadyExists(args.Name);
        return true;
    }
    if (!args.Attr && HasProtoFlag(flags, NProto::TSetNodeXAttrRequest::F_REPLACE)) {
        args.Error = ErrorAttributeNotExists(args.Name);
        return true;
    }

    return true;
}

void TIndexTabletActor::ExecuteTx_SetNodeXAttr(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TSetNodeXAttr& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(SetNodeXAttr, args);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GenerateCommitId();

    if (args.Attr) {
        args.Version = UpdateNodeAttr(
            db,
            args.NodeId,
            args.Attr->MinCommitId,
            args.CommitId,
            *args.Attr,
            args.Value);
    } else {
        args.Version = CreateNodeAttr(
            db,
            args.NodeId,
            args.CommitId,
            args.Name,
            args.Value);
    }
}

void TIndexTabletActor::CompleteTx_SetNodeXAttr(
    const TActorContext& ctx,
    TTxIndexTablet::TSetNodeXAttr& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] SetNodeXAttr completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    if (SUCCEEDED(args.Error.GetCode())) {
        NProto::TSessionEvent sessionEvent;
        {
            auto* changed = sessionEvent.AddNodeChanged();
            changed->SetNodeId(args.NodeId);
            changed->SetKind(NProto::TSessionEvent::NODE_XATTR_CHANGED);
        }
        NotifySessionEvent(ctx, sessionEvent);
    }

    auto response = std::make_unique<TEvService::TEvSetNodeXAttrResponse>(args.Error);
    response->Record.SetVersion(args.Version);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
