#include "part2_actor.h"

#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>

namespace NCloud::NBlockStore::NStorage::NPartition2 {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

class TForcedCleanupActor final
    : public TActorBootstrapped<TForcedCleanupActor>
{
private:
    const TActorId Tablet;
    const TDuration RetryTimeout;

public:
    TForcedCleanupActor(
        const TActorId& tablet,
        TDuration retryTimeout);

    void Bootstrap(const TActorContext& ctx);

private:
    void SendCleanupRequest(const TActorContext& ctx);

    void NotifyCompleted(
        const TActorContext& ctx,
        const NProto::TError& error = {});

private:
    STFUNC(StateWork);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void HandleWakeup(
        const TEvents::TEvWakeup::TPtr& ev,
        const TActorContext& ctx);

    void HandleCleanupResponse(
        const TEvPartitionPrivate::TEvCleanupResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TForcedCleanupActor::TForcedCleanupActor(
        const TActorId& tablet,
        TDuration retryTimeout)
    : Tablet(tablet)
    , RetryTimeout(retryTimeout)
{
    ActivityType = TBlockStoreActivities::PARTITION_WORKER;
}

void TForcedCleanupActor::Bootstrap(const TActorContext& ctx)
{
    SendCleanupRequest(ctx);
    Become(&TThis::StateWork);
}

void TForcedCleanupActor::SendCleanupRequest(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvPartitionPrivate::TEvCleanupRequest>(
        MakeIntrusive<TCallContext>());

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TForcedCleanupActor::NotifyCompleted(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    auto response = std::make_unique<TEvPartitionPrivate::TEvForcedCleanupCompleted>(error);

    NCloud::Send(ctx, Tablet, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TForcedCleanupActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvWakeup, HandleWakeup);
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvPartitionPrivate::TEvCleanupResponse, HandleCleanupResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION_WORKER);
            break;
    }
}

void TForcedCleanupActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    SendCleanupRequest(ctx);
}

void TForcedCleanupActor::HandleCleanupResponse(
    const TEvPartitionPrivate::TEvCleanupResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    ui32 errorCode = msg->GetStatus();
    if (FAILED(errorCode)) {
        if (errorCode == E_TRY_AGAIN) {
            ctx.Schedule(RetryTimeout, new TEvents::TEvWakeup());
            return;
        }
        NotifyCompleted(ctx, msg->GetError());
        return;
    }

    NotifyCompleted(ctx);
}

void TForcedCleanupActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    auto error = MakeError(E_REJECTED, "Tablet is dead");

    NotifyCompleted(ctx, error);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleHttpInfo_ForceCleanup(
    const TActorContext& ctx,
    TRequestInfoPtr requestInfo,
    const TCgiParameters& params)
{
    Y_UNUSED(params);

    using namespace NMonitoringUtils;

    TString message;
    if (!State->IsForcedCleanupRunning()) {
        State->StartForcedCleanup();

        auto actorId = NCloud::Register<TForcedCleanupActor>(
            ctx,
            SelfId(),
            Config->GetCleanupRetryTimeout());

        Actors.insert(actorId);

        message = "Cleanup has been started";
    } else {
        message = "Cleanup is already running";
    }

    TStringStream out;
    BuildTabletNotifyPageWithRedirect(out, message, TabletID());

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());
    NCloud::Reply(ctx, *requestInfo, std::move(response));
}

void TPartitionActor::HandleForcedCleanupCompleted(
    const TEvPartitionPrivate::TEvForcedCleanupCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    Y_UNUSED(ctx);

    State->ResetForcedCleanup();
    Actors.erase(ev->Sender);
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition2
