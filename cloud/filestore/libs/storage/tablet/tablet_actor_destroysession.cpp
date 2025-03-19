#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleDestroySession(
    const TEvIndexTablet::TEvDestroySessionRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    const auto& clientId = GetClientId(msg->Record);
    const auto& sessionId = GetSessionId(msg->Record);

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] DestroySession c:%s, s:%s",
        TabletID(),
        clientId.Quote().c_str(),
        sessionId.Quote().c_str());

    auto* session = FindSession(sessionId);
    if (!session) {
        auto response = std::make_unique<TEvIndexTablet::TEvDestroySessionResponse>(
            MakeError(S_FALSE, "invalid session"));

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    if (session->GetClientId() != clientId) {
        auto response = std::make_unique<TEvIndexTablet::TEvDestroySessionResponse>(
            ErrorInvalidSession(clientId, sessionId));

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TDestroySession>(
        ctx,
        std::move(requestInfo),
        sessionId);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_DestroySession(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TDestroySession& args)
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

void TIndexTabletActor::ExecuteTx_DestroySession(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TDestroySession& args)
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

    RemoveSession(db, args.SessionId);
}

void TIndexTabletActor::CompleteTx_DestroySession(
    const TActorContext& ctx,
    TTxIndexTablet::TDestroySession& args)
{
    auto response = std::make_unique<TEvIndexTablet::TEvDestroySessionResponse>();
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
