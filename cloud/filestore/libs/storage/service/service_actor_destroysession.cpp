#include "service_actor.h"

#include <cloud/filestore/libs/storage/api/tablet.h>
#include <cloud/filestore/libs/storage/api/tablet_proxy.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TDestroySessionActor final
    : public TActorBootstrapped<TDestroySessionActor>
{
private:
    const TStorageConfigPtr Config;
    const TRequestInfoPtr RequestInfo;
    const TString ClientId;
    const TString FileSystemId;
    const TString SessionId;
    const TInstant Deadline;

public:
    TDestroySessionActor(
        TStorageConfigPtr config,
        TRequestInfoPtr requestInfo,
        TString clientId,
        TString fileSystemId,
        TString sessionId);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void DestroySession(const TActorContext& ctx);
    void HandleDestroySessionResponse(
        const TEvIndexTablet::TEvDestroySessionResponse::TPtr& ev,
        const TActorContext& ctx);

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
};

////////////////////////////////////////////////////////////////////////////////

TDestroySessionActor::TDestroySessionActor(
        TStorageConfigPtr config,
        TRequestInfoPtr requestInfo,
        TString clientId,
        TString fileSystemId,
        TString sessionId)
    : Config(std::move(config))
    , RequestInfo(std::move(requestInfo))
    , ClientId(std::move(clientId))
    , FileSystemId(std::move(fileSystemId))
    , SessionId(std::move(sessionId))
    , Deadline(Config->GetIdleSessionTimeout().ToDeadLine())
{
    ActivityType = TFileStoreActivities::SERVICE_WORKER;
}

void TDestroySessionActor::Bootstrap(const TActorContext& ctx)
{
    DestroySession(ctx);
    Become(&TThis::StateWork);
}

void TDestroySessionActor::DestroySession(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvIndexTablet::TEvDestroySessionRequest>();
    request->Record.SetFileSystemId(FileSystemId);

    auto* headers = request->Record.MutableHeaders();
    headers->SetClientId(ClientId);
    headers->SetSessionId(SessionId);

    NCloud::Send(ctx, MakeIndexTabletProxyServiceId(), std::move(request));
}

void TDestroySessionActor::HandleDestroySessionResponse(
    const TEvIndexTablet::TEvDestroySessionResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_DEBUG(ctx, TFileStoreComponents::SERVICE_WORKER,
        "[%s] client removed (%s)",
        SessionId.Quote().c_str(),
        FormatError(msg->GetError()).c_str());

    if (msg->GetStatus() == E_REJECTED) {
        // Pipe error
        if (ctx.Now() < Deadline) {
            return ctx.Schedule(TDuration::Seconds(1), new TEvents::TEvWakeup());
        }
    }

    return ReplyAndDie(ctx, msg->GetError());
}

void TDestroySessionActor::HandleWakeUp(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    DestroySession(ctx);
}

void TDestroySessionActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    LOG_DEBUG(ctx, TFileStoreComponents::SERVICE_WORKER,
        "[%s] destroy poisoned, dying",
        SessionId.Quote().c_str());

    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TDestroySessionActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    Notify(ctx, error);
    Die(ctx);
}

void TDestroySessionActor::Notify(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    LOG_INFO(ctx, TFileStoreComponents::SERVICE_WORKER,
        "[%s] session stop completed (%s)",
        SessionId.Quote().c_str(),
        FormatError(error).c_str());

    {
        auto response = std::make_unique<TEvService::TEvDestroySessionResponse>(error);
        NCloud::Reply(ctx, *RequestInfo, std::move(response));
    }

    {
        auto response = std::make_unique<TEvServicePrivate::TEvSessionDestroyed>(error);
        response->SessionId = SessionId;

        NCloud::Send(ctx, MakeStorageServiceId(), std::move(response));
    }
}

STFUNC(TDestroySessionActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvents::TEvWakeup, HandleWakeUp);

        HFunc(TEvIndexTablet::TEvDestroySessionResponse, HandleDestroySessionResponse);
        IgnoreFunc(TEvServicePrivate::TEvPingSession);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::SERVICE_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TStorageServiceActor::HandleDestroySession(
    const TEvService::TEvDestroySessionRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    const auto& clientId = GetClientId(msg->Record);
    const auto& sessionId = GetSessionId(msg->Record);

    auto* session = State->FindSession(sessionId);
    if (!session) {
        auto response = std::make_unique<TEvService::TEvDestroySessionResponse>(
            MakeError(S_ALREADY, "session doesn't exist"));
        return NCloud::Reply(ctx, *ev, std::move(response));
    }

    if (session->ClientId != clientId || session->FileSystemId != msg->Record.GetFileSystemId()) {
        auto response = std::make_unique<TEvService::TEvDestroySessionResponse>(
            ErrorInvalidSession(clientId, sessionId));
        return NCloud::Reply(ctx, *ev, std::move(response));
    }

    if (session->ShouldStop) {
        auto response = std::make_unique<TEvService::TEvDestroySessionResponse>(
            MakeError(E_REJECTED, "session destruction is in progress"));
        return NCloud::Reply(ctx, *ev, std::move(response));
    }

    LOG_INFO(ctx, TFileStoreComponents::SERVICE,
        "[%s] DestroySession (client: %s, filesystem: %s)",
        sessionId.Quote().c_str(),
        clientId.Quote().c_str(),
        session->FileSystemId.Quote().c_str());

    NCloud::Send(
        ctx,
        session->SessionActor,
        std::make_unique<TEvents::TEvPoisonPill>());

    session->SessionActor = {};
    session->ShouldStop = true;

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    auto actor = std::make_unique<TDestroySessionActor>(
        StorageConfig,
        std::move(requestInfo),
        session->ClientId,
        session->FileSystemId,
        session->SessionId);

    session->SessionActor = NCloud::Register(ctx, std::move(actor));
}

void TStorageServiceActor::HandleSessionDestroyed(
    const TEvServicePrivate::TEvSessionDestroyed::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_INFO(ctx, TFileStoreComponents::SERVICE,
        "[%s] DestroySession completed (%s)",
        msg->SessionId.Quote().c_str(),
        FormatError(msg->GetError()).c_str());

    auto* session = State->FindSession(msg->SessionId);
    if (session) {
        State->RemoveSession(session);
    } else {
        // shutdown or stop before start
    }
}

}   // namespace NCloud::NFileStore::NStorage
