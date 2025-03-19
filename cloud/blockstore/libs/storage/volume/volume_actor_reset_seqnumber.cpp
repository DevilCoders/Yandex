#include "volume_actor.h"

#include "volume_database.h"

#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;
using namespace NMonitoringUtils;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

class THttpResetMountSeqNumberActor final
    : public TActorBootstrapped<THttpResetMountSeqNumberActor>
{
private:
    const TActorId VolumeActor;
    const TRequestInfoPtr RequestInfo;
    TString ClientId;
    const ui64 TabletId;

public:
    THttpResetMountSeqNumberActor(
        TActorId volumeActor,
        TRequestInfoPtr requestInfo,
        TString clientId,
        ui64 tabletId);

    void Bootstrap(const TActorContext& ctx);

private:
    void Notify(
        const TActorContext& ctx,
        const TString& message,
        const EAlertLevel alertLevel);

private:
    STFUNC(StateWork);

    void HandleResetResponse(
        const TEvVolumePrivate::TEvResetMountSeqNumberResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleResetMountSeqNumber(
        const TEvVolumePrivate::TEvResetMountSeqNumberRequest::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

THttpResetMountSeqNumberActor::THttpResetMountSeqNumberActor(
        TActorId volumeActor,
        TRequestInfoPtr requestInfo,
        TString clientId,
        ui64 tabletId)
    : VolumeActor(volumeActor)
    , RequestInfo(std::move(requestInfo))
    , ClientId(std::move(clientId))
    , TabletId(tabletId)
{
    ActivityType = TBlockStoreActivities::VOLUME;
}

void THttpResetMountSeqNumberActor::Bootstrap(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvVolumePrivate::TEvResetMountSeqNumberRequest>(
        std::move(ClientId));

    NCloud::SendWithUndeliveryTracking(ctx, VolumeActor, std::move(request));

    Become(&TThis::StateWork);
}

void THttpResetMountSeqNumberActor::Notify(
    const TActorContext& ctx,
    const TString& message,
    const EAlertLevel alertLevel)
{
    TStringStream out;
    BuildTabletNotifyPageWithRedirect(out, message, TabletId, alertLevel);

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void THttpResetMountSeqNumberActor::HandleResetResponse(
    const TEvVolumePrivate::TEvResetMountSeqNumberResponse::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ctx);

    if (FAILED(ev->Get()->GetStatus())) {
        Notify(
            ctx,
            TStringBuilder() << "Operation failed: " << FormatError(ev->Get()->GetError()),
            EAlertLevel::DANGER);
    } else {
        Notify(ctx, "Operation completed successfully", EAlertLevel::SUCCESS);
    }

    Die(ctx);
}

void THttpResetMountSeqNumberActor::HandleResetMountSeqNumber(
    const TEvVolumePrivate::TEvResetMountSeqNumberRequest::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    Y_UNUSED(ctx);

    Notify(ctx, "tablet is dead", EAlertLevel::DANGER);

    Die(ctx);
}

void THttpResetMountSeqNumberActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    Y_UNUSED(ctx);

    Notify(ctx, "tablet is dead", EAlertLevel::DANGER);

    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(THttpResetMountSeqNumberActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvVolumePrivate::TEvResetMountSeqNumberResponse, HandleResetResponse);
        HFunc(TEvVolumePrivate::TEvResetMountSeqNumberRequest, HandleResetMountSeqNumber);

        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME);
        break;
    }
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleResetMountSeqNumber(
    const TEvVolumePrivate::TEvResetMountSeqNumberRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();
    auto& clientId = msg->ClientId;

    auto requestInfo = CreateRequestInfo<TEvVolumePrivate::TResetMountSeqNumberMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    AddTransaction(*requestInfo);

    ExecuteTx<TResetMountSeqNumber>(
        ctx,
        std::move(requestInfo),
        std::move(clientId));
}

void TVolumeActor::HandleHttpInfo_ResetMountSeqNumber(
    const NMon::TEvRemoteHttpInfo::TPtr& ev,
    const TString& clientId,
    const TActorContext& ctx)
{
    if (!State) {
        TStringStream out;
        TString message = "Unable to reset mount seqnumber: volume state is not initialized";
        BuildTabletNotifyPageWithRedirect(
            out,
            message,
            TabletID(),
            EAlertLevel::DANGER);

        NCloud::Reply(
            ctx,
            *ev,
            std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str()));
        return;
    }

    if (ev->Get()->GetMethod() != HTTP_METHOD_POST) {
        RejectHttpRequest(ev->Sender, ctx);
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] resetting mount seqnumber from monitoring page: volume %s",
        TabletID(),
        State->GetDiskId().Quote().data());

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>(),
        std::move(ev->TraceId));

    auto actorId = NCloud::Register<THttpResetMountSeqNumberActor>(
        ctx,
        SelfId(),
        std::move(requestInfo),
        clientId,
        TabletID());

    Actors.insert(actorId);
}

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::PrepareResetMountSeqNumber(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TResetMountSeqNumber& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(args);

    TVolumeDatabase db(tx.DB);
    return db.ReadClient(args.ClientId, args.ClientInfo);
}

void TVolumeActor::ExecuteResetMountSeqNumber(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TResetMountSeqNumber& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(args);

    TVolumeDatabase db(tx.DB);

    args.ClientInfo->SetMountSeqNumber(0);
    db.WriteClient(*args.ClientInfo);

    State->SetMountSeqNumber(0);
}

void TVolumeActor::CompleteResetMountSeqNumber(
    const TActorContext& ctx,
    TTxVolume::TResetMountSeqNumber& args)
{
    auto response = std::make_unique<TEvVolumePrivate::TEvResetMountSeqNumberResponse>();

    BLOCKSTORE_TRACE_SENT(ctx, &args.RequestInfo->TraceId, this, response);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

    RemoveTransaction(*args.RequestInfo);
    Actors.erase(args.RequestInfo->Sender);
}

}   // namespace NCloud::NBlockStore::NStorage
