#include "service_actor.h"
#include "service.h"

#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NMonitoringUtils;

namespace {

////////////////////////////////////////////////////////////////////////////////

using EChangeBindingOp = TEvService::TEvChangeVolumeBindingRequest::EChangeBindingOp;

class THttpVolumeBindingActor final
    : public TActorBootstrapped<THttpVolumeBindingActor>
{
private:
    const TRequestInfoPtr RequestInfo;

    const TString DiskId;
    const bool Push;

public:
    THttpVolumeBindingActor(
        TRequestInfoPtr requestInfo,
        TString diskId,
        bool push);

    void Bootstrap(const TActorContext& ctx);

private:
    void Notify(
        const TActorContext& ctx,
        const TString& message,
        const EAlertLevel alertLevel);

private:
    STFUNC(StateWork);

    void HandleChangeVolumeBindingResponse(
        const TEvService::TEvChangeVolumeBindingResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

THttpVolumeBindingActor::THttpVolumeBindingActor(
        TRequestInfoPtr requestInfo,
        TString diskId,
        bool push)
    : RequestInfo(std::move(requestInfo))
    , DiskId(std::move(diskId))
    , Push(push)
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void THttpVolumeBindingActor::Bootstrap(const TActorContext& ctx)
{
    EChangeBindingOp action;
    if (Push) {
        action = EChangeBindingOp::RELEASE_TO_HIVE;
    } else {
        action = EChangeBindingOp::ACQUIRE_FROM_HIVE;
    }

    auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
        DiskId,
        action,
        NProto::EPreemptionSource::SOURCE_MANUAL);

    NCloud::Send(
        ctx,
        MakeStorageServiceId(),
        std::move(request));

    Become(&TThis::StateWork);
}

void THttpVolumeBindingActor::Notify(
    const TActorContext& ctx,
    const TString& message,
    const EAlertLevel alertLevel)
{
    TStringStream out;
    BuildNotifyPageWithRedirect(
        out,
        message,
        TStringBuilder()
            << "../blockstore/service",
        alertLevel);

    auto response = std::make_unique<NMon::TEvHttpInfoRes>(out.Str());
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void THttpVolumeBindingActor::HandleChangeVolumeBindingResponse(
    const TEvService::TEvChangeVolumeBindingResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* response = ev->Get();

    if (SUCCEEDED(response->GetStatus())) {
        Notify(ctx, "Operation completed successfully", EAlertLevel::SUCCESS);
    } else {
        Notify(
            ctx,
            TStringBuilder()
                << "failed to change volume binding"
                << DiskId.Quote()
                << ": " << FormatError(response->GetError()),
            EAlertLevel::DANGER);
    }

    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(THttpVolumeBindingActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvService::TEvChangeVolumeBindingResponse, HandleChangeVolumeBindingResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleHttpInfo_VolumeBinding(
    const NMon::TEvHttpInfo::TPtr& ev,
    const TString& diskId,
    const TString& actionType,
    const TActorContext& ctx)
{
    if (ev->Get()->Request.GetMethod() != HTTP_METHOD_POST) {
        RejectHttpRequest(ev->Sender, ctx);
        return;
    }

    LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
        "Changing volume binding per action from monitoring page: volume %s, action %s",
        diskId.Quote().data(),
        actionType.Quote().data());

    auto volume = State.GetVolume(diskId);
    if (!volume) {
        TString error = TStringBuilder()
            << "Failed to preempt volume: volume " << diskId << " not found";
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE, error.data());

        TStringStream out;
        BuildNotifyPageWithRedirect(
            out,
            error,
            TStringBuilder() << "../blockstore/service",
            EAlertLevel::DANGER);

        auto response = std::make_unique<NMon::TEvHttpInfoRes>(out.Str());
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>(),
        std::move(ev->TraceId));

    bool pushRequired = true;

    if (actionType == "Pull") {
        pushRequired = false;
    }

    NCloud::Register<THttpVolumeBindingActor>(
        ctx,
        std::move(requestInfo),
        diskId,
        pushRequired);
}

}   // namespace NCloud::NBlockStore::NStorage
