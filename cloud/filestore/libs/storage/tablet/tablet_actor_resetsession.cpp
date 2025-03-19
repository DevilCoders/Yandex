#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleResetSession(
    const TEvService::TEvResetSessionRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    const auto& clientId = GetClientId(msg->Record);
    const auto& sessionId = GetSessionId(msg->Record);

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] ResetSession c:%s, s:%s",
        TabletID(),
        clientId.Quote().c_str(),
        sessionId.Quote().c_str());

    auto* session = FindSession(sessionId);
    if (!session || session->GetClientId() != clientId) {
        auto response = std::make_unique<TEvService::TEvResetSessionResponse>(
            ErrorInvalidSession(clientId, sessionId));

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TResetSession>(
        ctx,
        std::move(requestInfo),
        sessionId,
        msg->Record.GetSessionState());
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_ResetSession(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TResetSession& args)
{
    Y_UNUSED(ctx);

    auto* session = FindSession(args.SessionId);
    if (!session) {
        return true;
    }

    TIndexTabletDatabase db(tx.DB);

    bool ready = true;
    auto commitId = GetCurrentCommitId();
    args.Nodes.reserve(session->Handles.Size());
    for (const auto& handle: session->Handles) {
        if (args.Nodes.contains(handle.GetNodeId())) {
            continue;
        }

        TMaybe<TIndexTabletDatabase::TNode> node;
        if (!ReadNode(db, handle.GetNodeId(), commitId, node)) {
            ready = false;
        } else {
            Y_VERIFY(node);
            if (node->Attrs.GetLinks() == 0) {
                // candidate to be removed
                args.Nodes.insert(*node);
            }
        }
    }

    return ready;
}

void TIndexTabletActor::ExecuteTx_ResetSession(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TResetSession& args)
{
    Y_UNUSED(ctx);

    TIndexTabletDatabase db(tx.DB);

    auto* session = FindSession(args.SessionId);
    if (!session) {
        return;
    }

    auto commitId = GenerateCommitId();

    auto handle = session->Handles.begin();
    while (handle != session->Handles.end()) {
        auto nodeId = handle->GetNodeId();
        DestroyHandle(db, &*(handle++));

        auto it = args.Nodes.find(nodeId);
        if (it != args.Nodes.end() && !HasOpenHandles(nodeId)) {
            RemoveNode(
                db,
                *it,
                it->MinCommitId,
                commitId);
        }
    }

    ResetSession(db, session, args.SessionState);
}

void TIndexTabletActor::CompleteTx_ResetSession(
    const TActorContext& ctx,
    TTxIndexTablet::TResetSession& args)
{
    auto response = std::make_unique<TEvService::TEvResetSessionResponse>();
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
