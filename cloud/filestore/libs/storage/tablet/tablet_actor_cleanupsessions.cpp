#include "tablet_actor.h"

#include <library/cpp/actors/core/actor_bootstrapped.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;
using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TCleanupSessionsActor final
    : public TActorBootstrapped<TCleanupSessionsActor>
{
private:
    const TActorId Tablet;
    const TRequestInfoPtr RequestInfo;
    const TVector<NProto::TSession> Sessions;

    size_t ResponsesCollected = 0;

public:
    TCleanupSessionsActor(
        const TActorId& tablet,
        TRequestInfoPtr requestInfo,
        TVector<NProto::TSession> sessions);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void DestroySessions(const TActorContext& ctx);
    void HandleSessionDestroyed(
        const TEvIndexTablet::TEvDestroySessionResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        const NProto::TError& error = {});
};

////////////////////////////////////////////////////////////////////////////////

TCleanupSessionsActor::TCleanupSessionsActor(
        const TActorId& tablet,
        TRequestInfoPtr requestInfo,
        TVector<NProto::TSession> sessions)
    : Tablet(tablet)
    , RequestInfo(std::move(requestInfo))
    , Sessions(std::move(sessions))
{
    ActivityType = TFileStoreComponents::TABLET_WORKER;
}

void TCleanupSessionsActor::Bootstrap(const TActorContext& ctx)
{
    DestroySessions(ctx);
    Become(&TThis::StateWork);
}

void TCleanupSessionsActor::DestroySessions(const TActorContext& ctx)
{
    for (size_t i = 0; i < Sessions.size(); ++i) {
        LOG_DEBUG(ctx, TFileStoreComponents::TABLET_WORKER,
            "[%s] initiating session cleanup",
            Sessions[i].GetSessionId().c_str());

        auto request = std::make_unique<TEvIndexTablet::TEvDestroySessionRequest>();
        request->CallContext = MakeIntrusive<TCallContext>();

        auto* headers = request->Record.MutableHeaders();
        headers->SetSessionId(Sessions[i].GetSessionId());
        headers->SetClientId(Sessions[i].GetClientId());

        NCloud::Send(ctx, Tablet, std::move(request), i);
    }
}

void TCleanupSessionsActor::HandleSessionDestroyed(
    const TEvIndexTablet::TEvDestroySessionResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    Y_VERIFY(ev->Cookie < Sessions.size());
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET_WORKER,
        "[%s] session cleaned: (error %s)",
        Sessions[ev->Cookie].GetSessionId().c_str(),
        FormatError(msg->GetError()).c_str());

    if (++ResponsesCollected == Sessions.size()) {
        ReplyAndDie(ctx);
    }
}

void TCleanupSessionsActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TCleanupSessionsActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    {
        // notify tablet
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvCleanupSessionsCompleted>(error);
        NCloud::Send(ctx, Tablet, std::move(response));
    }

    if (RequestInfo->Sender != Tablet) {
        // reply to caller
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvCleanupSessionsResponse>(error);
        NCloud::Reply(ctx, *RequestInfo, std::move(response));
    }

    Die(ctx);
}

STFUNC(TCleanupSessionsActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvIndexTablet::TEvDestroySessionResponse, HandleSessionDestroyed);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::ScheduleCleanupSessions(const TActorContext& ctx)
{
    if (!CleanupSessionsScheduled) {
        ctx.Schedule(
            Config->GetIdleSessionTimeout(),
            new TEvIndexTabletPrivate::TEvCleanupSessionsRequest());
        CleanupSessionsScheduled = true;
    }
}

void TIndexTabletActor::HandleCleanupSessions(
    const TEvIndexTabletPrivate::TEvCleanupSessionsRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    if (ev->Sender == ctx.SelfID) {
        CleanupSessionsScheduled = false;
    }

    auto sessions = GetTimeoutedSessions(ctx.Now());
    if (!sessions) {
        // nothing to do
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvCleanupSessionsResponse>();
        NCloud::Reply(ctx, *ev, std::move(response));

        ScheduleCleanupSessions(ctx);
        return;
    }

    TVector<NProto::TSession> list(sessions.size());
    for (size_t i = 0; i < sessions.size(); ++i) {
        list[i].CopyFrom(*sessions[i]);
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    auto actor = std::make_unique<TCleanupSessionsActor>(
        ctx.SelfID,
        std::move(requestInfo),
        std::move(list));

    auto actorId = NCloud::Register(ctx, std::move(actor));
    WorkerActors.insert(actorId);
}

void TIndexTabletActor::HandleCleanupSessionsCompleted(
    const TEvIndexTabletPrivate::TEvCleanupSessionsCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] CleanupSessions completed (%s)",
        TabletID(),
        FormatError(msg->GetError()).c_str());

    WorkerActors.erase(ev->Sender);
    ScheduleCleanupSessions(ctx);
}

}   // namespace NCloud::NFileStore::NStorage
