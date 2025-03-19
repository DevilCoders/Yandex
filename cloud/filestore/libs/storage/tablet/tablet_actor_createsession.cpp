#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

void DoRecoverSession(
    TIndexTabletState& state,
    TSession* session,
    const TActorId& owner,
    const TActorContext& ctx)
{
    auto oldOwner = state.RecoverSession(session, owner);
    if (oldOwner) {
        NCloud::Send(ctx, oldOwner, std::make_unique<TEvents::TEvPoisonPill>());
    }
}

void Convert(const NProto::TFileSystem& fileSystem, NProto::TFileStore& fileStore)
{
    fileStore.SetFileSystemId(fileSystem.GetFileSystemId());
    fileStore.SetProjectId(fileSystem.GetProjectId());
    fileStore.SetFolderId(fileSystem.GetFolderId());
    fileStore.SetCloudId(fileSystem.GetCloudId());
    fileStore.SetBlockSize(fileSystem.GetBlockSize());
    fileStore.SetBlocksCount(fileSystem.GetBlocksCount());
    // TODO need set ConfigVersion?
    fileStore.SetNodesCount(fileSystem.GetNodesCount());
    fileStore.SetStorageMediaKind(fileSystem.GetStorageMediaKind());
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleCreateSession(
    const TEvIndexTablet::TEvCreateSessionRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "%s CreateSession: %s",
        LogTag().c_str(),
        DumpMessage(msg->Record).c_str());

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TCreateSession>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_CreateSession(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCreateSession& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TIndexTabletActor::ExecuteTx_CreateSession(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCreateSession& args)
{
    Y_UNUSED(ctx);

    const auto& clientId = GetClientId(args.Request);
    const auto& sessionId = GetSessionId(args.Request);
    const auto& checkpointId = args.Request.GetCheckpointId();
    const auto owner = args.RequestInfo->Sender;

    // check if client reconnecting with known session id
    auto* session = FindSession(sessionId);
    if (session) {
        if (session->GetClientId() == clientId) {
            LOG_INFO(ctx, TFileStoreComponents::TABLET,
                "%s CreateSession c:%s, s:%s recovered by session",
                LogTag().c_str(),
                clientId.Quote().c_str(),
                session->GetSessionId().Quote().c_str());

            args.SessionId = session->GetSessionId();
            DoRecoverSession(*this, session, owner, ctx);
        } else {
            args.Error = MakeError(E_FS_INVALID_SESSION, "session clien id mismatch");
        }

        return;
    }

    // check if there is existing session for the client
    if (args.Request.GetRestoreClientSession()) {
        auto* session = FindSessionByClientId(clientId);
        if (session) {
            LOG_INFO(ctx, TFileStoreComponents::TABLET,
                "%s CreateSession c:%s, s:%s recovered by client",
                LogTag().c_str(),
                clientId.Quote().c_str(),
                session->GetSessionId().Quote().c_str());

            args.SessionId = session->GetSessionId();
            DoRecoverSession(*this, session, owner, ctx);

            return;
        } else {
            LOG_INFO(ctx, TFileStoreComponents::TABLET,
                "%s CreateSession: no session available for client c: %s",
                LogTag().c_str(),
                clientId.Quote().c_str());
        }
    }

    if (!sessionId) {
        args.Error = MakeError(E_ARGUMENT, "empty session id");
        return;
    }

    args.SessionId = sessionId;
    LOG_INFO(ctx, TFileStoreComponents::TABLET,
        "%s CreateSession c:%s, s:%s creating new session",
        LogTag().c_str(),
        clientId.Quote().c_str(),
        args.SessionId.Quote().c_str(),
        clientId.Quote().c_str(),
        FormatError(args.Error).c_str());

    TIndexTabletDatabase db(tx.DB);
    CreateSession(db, clientId, args.SessionId, checkpointId, owner);
}

void TIndexTabletActor::CompleteTx_CreateSession(
    const TActorContext& ctx,
    TTxIndexTablet::TCreateSession& args)
{
    LOG_INFO(ctx, TFileStoreComponents::TABLET,
        "%s CreateSession completed (%s)",
        LogTag().c_str(),
        FormatError(args.Error).c_str());

    if (HasError(args.Error)) {
        auto response = std::make_unique<TEvIndexTablet::TEvCreateSessionResponse>(args.Error);
        NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
        return;
    }

    auto session = FindSession(args.SessionId);
    Y_VERIFY(session);

    auto response = std::make_unique<TEvIndexTablet::TEvCreateSessionResponse>(args.Error);
    response->Record.SetSessionId(std::move(args.SessionId));
    response->Record.SetSessionState(session->GetSessionState());
    Convert(GetFileSystem(), *response->Record.MutableFileStore());

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
