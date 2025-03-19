#include "service_actor.h"

#include <cloud/blockstore/libs/kikimr/trace.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/mount_token.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>
#include <cloud/blockstore/libs/storage/service/service_events_private.h>

#include <cloud/storage/core/libs/api/hive_proxy.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/datetime/base.h>
#include <util/generic/deque.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

using namespace NCloud::NStorage;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TAddClientActor final
    : public TActorBootstrapped<TAddClientActor>
{
private:
    std::unique_ptr<TEvVolume::TEvAddClientRequest> Request;
    const TRequestInfoPtr RequestInfo;
    const TDuration Timeout;

    TActorId VolumeProxy;

public:
    TAddClientActor(
        std::unique_ptr<TEvVolume::TEvAddClientRequest> request,
        TRequestInfoPtr requestInfo,
        TDuration timeout,
        TActorId volumeProxy)
        : Request(std::move(request))
        , RequestInfo(std::move(requestInfo))
        , Timeout(timeout)
        , VolumeProxy(volumeProxy)
    {}

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void AddClient(const TActorContext& ctx);

    void HandleVolumeAddClientResponse(
        const TEvVolume::TEvAddClientResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleWakeup(
        const TEvents::TEvWakeup::TPtr& ev,
        const TActorContext& ctx);

    void ReplyErrorAndDie(
        const TActorContext& ctx,
        const NProto::TError& error = {});
};

////////////////////////////////////////////////////////////////////////////////

void TAddClientActor::Bootstrap(const TActorContext& ctx)
{
    Become(&TThis::StateWork);
    AddClient(ctx);
    ctx.Schedule(Timeout, new TEvents::TEvWakeup);
}

void TAddClientActor::AddClient(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvVolume::TEvAddClientRequest>();
    request->Record = Request->Record;

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Sending add client %s to volume %s",
        Request->Record.GetHeaders().GetClientId().Quote().data(),
        Request->Record.GetDiskId().Quote().data());

    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, request);

    ctx.Send(
        VolumeProxy,
        request.release(),
        0,
        RequestInfo->Cookie,
        RequestInfo->TraceId.Clone());
}

void TAddClientActor::HandleVolumeAddClientResponse(
    const TEvVolume::TEvAddClientResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, ev->Get(), &ev->TraceId);

    if (msg->Record.GetError().GetCode() == E_REJECTED) {
        AddClient(ctx);
        return;
    }
    ctx.Send(ev->Forward(RequestInfo->Sender));
    Die(ctx);
}

void TAddClientActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    LOG_WARN(ctx, TBlockStoreComponents::SERVICE,
        "AddClient %s request sent to volume %s timed out",
        Request->Record.GetHeaders().GetClientId().Quote().data(),
        Request->Record.GetDiskId().Quote().data());

    NCloud::Send<TEvServicePrivate::TEvResetPipeClient>(ctx, VolumeProxy);

    ReplyErrorAndDie(
        ctx,
        MakeError(E_REJECTED, "Failed to deliver add client request to volume"));
}

void TAddClientActor::ReplyErrorAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    auto response = std::make_unique<TEvVolume::TEvAddClientResponse>(error);

    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);
    NCloud::Send(ctx, RequestInfo->Sender, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TAddClientActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {

        HFunc(TEvVolume::TEvAddClientResponse, HandleVolumeAddClientResponse);
        HFunc(TEvents::TEvWakeup, HandleWakeup);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
        break;
    }
}

}    // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr CreateAddClientActor(
    std::unique_ptr<TEvVolume::TEvAddClientRequest> request,
    TRequestInfoPtr requestInfo,
    TDuration timeout,
    TActorId volumeProxy)
{
    return std::make_unique<TAddClientActor>(
        std::move(request),
        std::move(requestInfo),
        timeout,
        volumeProxy);
}

}   // namespace NCloud::NBlockStore::NStorage
