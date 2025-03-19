#include "helpers.h"
#include "service_actor.h"

#include <cloud/filestore/libs/storage/api/ss_proxy.h>
#include <cloud/filestore/libs/storage/api/tablet.h>

#include <ydb/core/base/tablet_pipe.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/datetime/base.h>
#include <util/generic/deque.h>
#include <util/generic/guid.h>
#include <util/generic/string.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

std::pair<ui64, ui64> GetSeqNo(const NProto::TGetSessionEventsResponse& response)
{
    size_t numEvents = response.EventsSize();
    Y_VERIFY(numEvents);

    return {
        response.GetEvents(0).GetSeqNo(),
        response.GetEvents(numEvents - 1).GetSeqNo(),
    };
}

////////////////////////////////////////////////////////////////////////////////

class TCreateSessionActor final
    : public TActorBootstrapped<TCreateSessionActor>
{
private:
    const TStorageConfigPtr Config;
    TRequestInfoPtr RequestInfo;

    const TString ClientId;
    const TString FileSystemId;
    const TString CheckpointId;
    const bool RestoreClientSession;
    TString SessionId;
    TString SessionState;
    NProto::TFileStore FileStore;

    ui64 TabletId = -1;
    TActorId PipeClient;

    TInstant LastPipeResetTime;
    TInstant LastPing;

    static const int MaxStoredEvents = 1000;
    google::protobuf::RepeatedPtrField<NProto::TSessionEvent> StoredEvents;
    TActorId EventListener;

public:
    TCreateSessionActor(
        TStorageConfigPtr config,
        TRequestInfoPtr requestInfo,
        TString clientId,
        TString fileSystemId,
        TString sessionId,
        TString checkpointId,
        bool restoreClientSession);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateResolve);
    STFUNC(StateWork);

    void DescribeFileStore(const TActorContext& ctx);
    void HandleDescribeFileStoreResponse(
        const TEvSSProxy::TEvDescribeFileStoreResponse::TPtr& ev,
        const TActorContext& ctx);

    void CreateSession(const TActorContext& ctx);
    void HandleCreateSessionResponse(
        const TEvIndexTablet::TEvCreateSessionResponse::TPtr& ev,
        const TActorContext& ctx);

    void CreatePipe(const TActorContext& ctx);

    void HandleConnect(
        TEvTabletPipe::TEvClientConnected::TPtr& ev,
        const TActorContext& ctx);

    void HandleDisconnect(
        TEvTabletPipe::TEvClientDestroyed::TPtr& ev,
        const TActorContext& ctx);

    void HandlePingSession(
        const TEvServicePrivate::TEvPingSession::TPtr& ev,
        const TActorContext& ctx);

    void HandleGetSessionEvents(
        const TEvService::TEvGetSessionEventsRequest::TPtr& ev,
        const TActorContext& ctx);

    void HandleGetSessionEventsResponse(
        const TEvService::TEvGetSessionEventsResponse::TPtr& ev,
        const TActorContext& ctx);

    void ScheduleWakeUp(const TActorContext& ctx);

    void HandleWakeUp(
        const TEvents::TEvWakeup::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        const NProto::TError& error);

    void Notify(
        const TActorContext& ctx,
        const NProto::TError& error);

    void Die(const TActorContext& ctx) override;

    TString LogTag()
    {
        return Sprintf("[f:%s][c:%s][s:%s]",
            FileSystemId.Quote().c_str(),
            ClientId.Quote().c_str(),
            SessionId.Quote().c_str());
    }
};

////////////////////////////////////////////////////////////////////////////////

TCreateSessionActor::TCreateSessionActor(
        TStorageConfigPtr config,
        TRequestInfoPtr requestInfo,
        TString clientId,
        TString fileSystemId,
        TString sessionId,
        TString checkpointId,
        bool restoreClientSession)
    : Config(std::move(config))
    , RequestInfo(std::move(requestInfo))
    , ClientId(std::move(clientId))
    , FileSystemId(std::move(fileSystemId))
    , CheckpointId(std::move(checkpointId))
    , RestoreClientSession(restoreClientSession)
    , SessionId(std::move(sessionId))
{
    ActivityType = TFileStoreActivities::SERVICE_WORKER;
}

void TCreateSessionActor::Bootstrap(const TActorContext& ctx)
{
    DescribeFileStore(ctx);
    Become(&TThis::StateResolve);
}

void TCreateSessionActor::DescribeFileStore(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvSSProxy::TEvDescribeFileStoreRequest>(
        FileSystemId);

    NCloud::Send(ctx, MakeSSProxyServiceId(), std::move(request));
}

void TCreateSessionActor::HandleDescribeFileStoreResponse(
    const TEvSSProxy::TEvDescribeFileStoreResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (FAILED(msg->GetStatus())) {
        return ReplyAndDie(ctx, msg->GetError());
    }

    const auto& pathDescr = msg->PathDescription;
    const auto& fsDescr = pathDescr.GetFileStoreDescription();

    TabletId = fsDescr.GetIndexTabletId();

    CreatePipe(ctx);
    ScheduleWakeUp(ctx);

    Become(&TThis::StateWork);
}

void TCreateSessionActor::CreatePipe(const TActorContext& ctx)
{
    LOG_INFO(ctx, TFileStoreComponents::SERVICE_WORKER,
        "%s creating pipe for tablet %lu, last reset at %lu",
        LogTag().c_str(),
        TabletId,
        LastPipeResetTime.Seconds());

    if (!LastPipeResetTime) {
        LastPipeResetTime = ctx.Now();
    }

    // TODO
    NTabletPipe::TClientConfig clientConfig;
    PipeClient = ctx.Register(CreateClient(SelfId(), TabletId, clientConfig));
}

void TCreateSessionActor::HandleConnect(
    TEvTabletPipe::TEvClientConnected::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    if (msg->Status != NKikimrProto::OK) {
        LOG_ERROR(ctx, TFileStoreComponents::SERVICE_WORKER,
            "%s failed to connect to %lu: %s",
            LogTag().c_str(),
            TabletId,
            NKikimrProto::EReplyStatus_Name(msg->Status).data());

        NTabletPipe::CloseClient(ctx, PipeClient);
        PipeClient = {};
        return;
    }

    LOG_INFO(ctx, TFileStoreComponents::SERVICE_WORKER,
        "%s pipe connected to %lu",
        LogTag().c_str(),
        TabletId);

    LastPipeResetTime = {};
    LastPing = ctx.Now();

    CreateSession(ctx);
}

void TCreateSessionActor::HandleDisconnect(
    TEvTabletPipe::TEvClientDestroyed::TPtr&,
    const TActorContext& ctx)
{
    LOG_INFO(ctx, TFileStoreComponents::SERVICE_WORKER,
        "%s pipe disconnected",
        LogTag().c_str(),
        TabletId);

    NTabletPipe::CloseClient(ctx, PipeClient);
    PipeClient = {};
}

void TCreateSessionActor::CreateSession(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvIndexTablet::TEvCreateSessionRequest>();
    request->Record.SetFileSystemId(FileSystemId);
    request->Record.SetCheckpointId(CheckpointId);
    request->Record.SetRestoreClientSession(RestoreClientSession);

    auto* headers = request->Record.MutableHeaders();
    headers->SetClientId(ClientId);
    headers->SetSessionId(SessionId);

    LOG_INFO(ctx, TFileStoreComponents::SERVICE_WORKER,
        "%s do creating session: %s",
        LogTag().c_str(),
        DumpMessage(request->Record).c_str());

    NTabletPipe::SendData(ctx, PipeClient, request.release());
}

void TCreateSessionActor::HandleCreateSessionResponse(
    const TEvIndexTablet::TEvCreateSessionResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    const auto& sessionId = msg->Record.GetSessionId();
    if (FAILED(msg->GetStatus())) {
        return ReplyAndDie(ctx, msg->GetError());
    } else if (!sessionId) {
        auto error = MakeError(E_FAIL, "empty session id");
        return ReplyAndDie(ctx, msg->GetError());
    } else if (sessionId != SessionId) {
        if (!RestoreClientSession) {
            auto error = MakeError(E_FAIL, "session id mismatch w/o restore flag");
            return ReplyAndDie(ctx, msg->GetError());
        }

        LOG_INFO(ctx, TFileStoreComponents::SERVICE_WORKER,
            "%s restored session id: actual id %s, state(%lu)",
            LogTag().c_str(),
            sessionId.Quote().c_str(),
            msg->Record.GetSessionState().size());

        SessionId = sessionId;
    } else {
        LOG_INFO(ctx, TFileStoreComponents::SERVICE_WORKER,
            "%s session established: %s, state(%lu)",
            LogTag().c_str(),
            FormatError(msg->GetError()).c_str(),
            msg->Record.GetSessionState().size());
    }

    SessionState = msg->Record.GetSessionState();
    FileStore = msg->Record.GetFileStore();

    Notify(ctx, {});
}

void TCreateSessionActor::HandlePingSession(
    const TEvServicePrivate::TEvPingSession::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    LastPing = ctx.Now();
}

void TCreateSessionActor::HandleGetSessionEvents(
    const TEvService::TEvGetSessionEventsRequest::TPtr& ev,
    const TActorContext& ctx)
{
    Y_VERIFY(SessionId);

    if (ev->Cookie == TEvService::StreamCookie) {
        LOG_INFO(ctx, TFileStoreComponents::SERVICE_WORKER,
            "%s subscribe event listener (%s)",
            LogTag().c_str(),
            ToString(EventListener).c_str());

        EventListener = ev->Sender;
    }

    auto response = std::make_unique<TEvService::TEvGetSessionEventsResponse>();
    response->Record.MutableEvents()->Swap(&StoredEvents);

    NCloud::Reply(ctx, *ev, std::move(response));
}

void TCreateSessionActor::HandleGetSessionEventsResponse(
    const TEvService::TEvGetSessionEventsResponse::TPtr& ev,
    const TActorContext& ctx)
{
    Y_VERIFY(SessionId);

    const auto* msg = ev->Get();

    const auto [firstSeqNo, lastSeqNo] = GetSeqNo(msg->Record);
    LOG_ERROR(ctx, TFileStoreComponents::SERVICE_WORKER,
        "%s got session events (seqNo: %lu-%lu)",
        LogTag().c_str(),
        firstSeqNo,
        lastSeqNo);

    if (EventListener) {
        Y_VERIFY(StoredEvents.size() == 0);

        // just forward response to listener as-is
        ctx.Send(ev->Forward(EventListener));
        return;
    }

    for (const auto& event: msg->Record.GetEvents()) {
        if (StoredEvents.size() < MaxStoredEvents) {
            StoredEvents.Add()->CopyFrom(event);
        } else {
            // number of stored events exceeded
            LOG_WARN(ctx, TFileStoreComponents::SERVICE_WORKER,
                "%s not enough space - session events will be lost (seqNo: %lu-%lu)",
                LogTag().c_str(),
                event.GetSeqNo(),
                lastSeqNo);
            break;
        }
    }
}

void TCreateSessionActor::ScheduleWakeUp(const TActorContext& ctx)
{
    ctx.Schedule(TDuration::Seconds(1), new TEvents::TEvWakeup());
}

void TCreateSessionActor::HandleWakeUp(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    auto idleTimeout = Config->GetIdleSessionTimeout();
    if (LastPing + idleTimeout < ctx.Now()) {
        LOG_INFO(ctx, TFileStoreComponents::SERVICE_WORKER,
            "%s closing idle session, last ping at %s",
            LogTag().c_str(),
            LastPing.ToStringUpToSeconds().c_str());

        return ReplyAndDie(ctx, MakeError(E_TIMEOUT, "closed idle session"));
    }

    auto connectTimeout = Config->GetEstablishSessionTimeout();
    if (LastPipeResetTime && LastPipeResetTime + connectTimeout < ctx.Now()) {
        LOG_WARN(ctx, TFileStoreComponents::SERVICE_WORKER,
            "%s create timeouted, couldn't connect from %s",
            LogTag().c_str(),
            LastPipeResetTime.ToStringUpToSeconds().c_str());

        return ReplyAndDie(ctx, MakeError(E_TIMEOUT, "failed to connect to fs"));
    }

    if (!PipeClient) {
        CreatePipe(ctx);
    }

    ScheduleWakeUp(ctx);
}

void TCreateSessionActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    LOG_DEBUG(ctx, TFileStoreComponents::SERVICE_WORKER,
        "%s session poisoned, dying",
        LogTag().c_str());

    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TCreateSessionActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    Notify(ctx, error);
    Die(ctx);
}

void TCreateSessionActor::Notify(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    Y_VERIFY(SessionId);
    LOG_INFO(ctx, TFileStoreComponents::SERVICE_WORKER,
        "%s session notify (%s)",
        LogTag().c_str(),
        FormatError(error).c_str());

    auto response = std::make_unique<TEvServicePrivate::TEvSessionCreated>(error);
    response->ClientId = ClientId;
    response->SessionId = SessionId;
    response->SessionState = SessionState;
    response->TabletId = TabletId;
    response->FileStore = FileStore;
    response->RequestInfo = std::move(RequestInfo);

    NCloud::Send(ctx, MakeStorageServiceId(), std::move(response));
}

void TCreateSessionActor::Die(const TActorContext& ctx)
{
    if (PipeClient) {
        NTabletPipe::CloseClient(ctx, PipeClient);
        PipeClient = {};
    }

    TActor::Die(ctx);
}

STFUNC(TCreateSessionActor::StateResolve)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvServicePrivate::TEvPingSession, HandlePingSession);
        HFunc(TEvSSProxy::TEvDescribeFileStoreResponse, HandleDescribeFileStoreResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::SERVICE_WORKER);
            break;
    }
}

STFUNC(TCreateSessionActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvents::TEvWakeup, HandleWakeUp);

        HFunc(TEvTabletPipe::TEvClientConnected, HandleConnect);
        HFunc(TEvTabletPipe::TEvClientDestroyed, HandleDisconnect);

        HFunc(TEvIndexTablet::TEvCreateSessionResponse, HandleCreateSessionResponse);
        HFunc(TEvServicePrivate::TEvPingSession, HandlePingSession);
        HFunc(TEvService::TEvGetSessionEventsRequest, HandleGetSessionEvents);
        HFunc(TEvService::TEvGetSessionEventsResponse, HandleGetSessionEventsResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::SERVICE_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TStorageServiceActor::HandleCreateSession(
    const TEvService::TEvCreateSessionRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    const auto& clientId = GetClientId(msg->Record);
    const auto& fileSystemId = msg->Record.GetFileSystemId();
    const auto& checkpointId = msg->Record.GetCheckpointId();

    auto sessionId = GetSessionId(msg->Record);
    if (auto session = State->FindSession(sessionId)) {
        auto code = (session->ClientId == clientId) ? S_ALREADY : E_FS_INVALID_SESSION;
        auto response = std::make_unique<TEvService::TEvCreateSessionResponse>(
            MakeError(code, "session already exists"));

        session->GetInfo(*response->Record.MutableSession());
        return NCloud::Reply(ctx, *ev, std::move(response));
    }

    if (!sessionId) {
        sessionId = CreateGuidAsString();
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_INFO(ctx, TFileStoreComponents::SERVICE,
        "[f:%s][c:%s][s:%s] create session",
        fileSystemId.Quote().c_str(),
        clientId.Quote().c_str(),
        sessionId.Quote().c_str());

    auto actor = std::make_unique<TCreateSessionActor>(
        StorageConfig,
        std::move(requestInfo),
        clientId,
        fileSystemId,
        sessionId,
        checkpointId,
        msg->Record.GetRestoreClientSession());

    NCloud::Register(ctx, std::move(actor));
}

void TStorageServiceActor::RemoveSession(TSessionInfo* session, const TActorContext& ctx)
{
    ctx.Send(session->SessionActor, new TEvents::TEvPoisonPill());
    State->RemoveSession(session);
}

void TStorageServiceActor::HandleSessionCreated(
    const TEvServicePrivate::TEvSessionCreated::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto* session = State->FindSession(msg->SessionId);
    if (SUCCEEDED(msg->GetStatus())) {
        // in case of vhost restart we don't know session id so inevitably will create new actor
        if (session && session->SessionActor != ev->Sender) {
            RemoveSession(session, ctx);
            session = nullptr;
        }

        if (!session) {
            LOG_INFO(ctx, TFileStoreComponents::SERVICE,
                "[f:%s][c:%s][s:%s] session created (%s), state(%lu)",
                msg->FileStore.GetFileSystemId().Quote().c_str(),
                msg->ClientId.Quote().c_str(),
                msg->SessionId.Quote().c_str(),
                FormatError(msg->GetError()).c_str(),
                msg->SessionState.size());

            ui64 requestId = 0;
            if (msg->RequestInfo && msg->RequestInfo->CallContext) {
                requestId = msg->RequestInfo->CallContext->RequestId;
            }

            session = State->CreateSession(
                msg->ClientId,
                msg->FileStore.GetFileSystemId(),
                msg->SessionId,
                msg->SessionState,
                ev->Sender,
                msg->TabletId);

            Y_VERIFY(session);
        }
    } else if (session) {
        // else it's an old notify from a dead actor
        if (session->SessionActor == ev->Sender) {
            // e.g. pipe failed or smth. client will have to restore it
            LOG_WARN(ctx, TFileStoreComponents::SERVICE,
                "[f:%s][c:%s][s:%s] session failed (%s)",
                msg->FileStore.GetFileSystemId().Quote().c_str(),
                msg->ClientId.Quote().c_str(),
                msg->SessionId.Quote().c_str(),
                FormatError(msg->GetError()).c_str());

            RemoveSession(session, ctx);
            session = nullptr;
        }
    } else {
        LOG_ERROR(ctx, TFileStoreComponents::SERVICE,
            "[f:%s][c:%s][s:%s] session creation failed (%s)",
            msg->FileStore.GetFileSystemId().Quote().c_str(),
            msg->ClientId.Quote().c_str(),
            msg->SessionId.Quote().c_str(),
            FormatError(msg->GetError()).c_str());
    }

    if (msg->RequestInfo) {
        auto response = std::make_unique<TEvService::TEvCreateSessionResponse>(msg->GetError());
        if (session) {
            session->GetInfo(*response->Record.MutableSession());
            response->Record.MutableFileStore()->CopyFrom(msg->FileStore);
        }

        NCloud::Reply(ctx, *msg->RequestInfo, std::move(response));
    }
}

}   // namespace NCloud::NFileStore::NStorage
