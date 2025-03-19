#include "volume_client_actor.h"

#include "service_events_private.h"

#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/kikimr/trace.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/service/service_events_private.h>

#include <cloud/storage/core/libs/diagnostics/trace_serializer.h>

#include <ydb/core/tablet/tablet_pipe_client_cache.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

Y_HAS_MEMBER(GetThrottlerDelay);

////////////////////////////////////////////////////////////////////////////////

NTabletPipe::TClientConfig CreateTabletPipeClientConfig(
    const TStorageConfig& config)
{
    NTabletPipe::TClientConfig clientConfig;
    clientConfig.RetryPolicy = {
        .RetryLimitCount = config.GetPipeClientRetryCount(),
        .MinRetryTime = config.GetPipeClientMinRetryTime(),
        .MaxRetryTime = config.GetPipeClientMaxRetryTime()
    };
    return clientConfig;
}

////////////////////////////////////////////////////////////////////////////////

class TVolumeClientActor final
    : public TActor<TVolumeClientActor>
{
    struct TActiveRequest
    {
        IEventHandlePtr Request;
        TCallContextPtr CallContext;
        ui64 SendTime;

        TActiveRequest(
                IEventHandlePtr request,
                TCallContextPtr callContext,
                ui64 sendTime)
            : Request(std::move(request))
            , CallContext(std::move(callContext))
            , SendTime(sendTime)
        {}
    };

    using TActiveRequestMap = THashMap<ui64, TActiveRequest>;

private:
    ITraceSerializerPtr TraceSerializer;
    const TActorId SessionActorId;
    const TString DiskId;
    const ui64 TabletId;
    const NTabletPipe::TClientConfig ClientConfig;

    ui32 Generation = 0;
    ui64 RequestId = 0;

    TActiveRequestMap ActiveRequests;

    TActorId PipeClient;

public:
    TVolumeClientActor(
        TStorageConfigPtr config,
        ITraceSerializerPtr traceSerializer,
        const TActorId& sessionActorId,
        TString diskId,
        ui64 tabletId);

private:
    void OnConnectionError(
        const TActorContext& ctx,
        const NProto::TError& error);

    void CancelActiveRequests(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void HandleConnect(
        TEvTabletPipe::TEvClientConnected::TPtr& ev,
        const TActorContext& ctx);

    void HandleDisconnect(
        TEvTabletPipe::TEvClientDestroyed::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void HandleResetPipeClient(
        TEvServicePrivate::TEvResetPipeClient::TPtr& ev,
        const TActorContext& ctx);

    template <typename TMethod>
    void HandleRequest(
        const TActorContext& ctx,
        const typename TMethod::TRequest::TPtr& ev);

    template <typename TMethod>
    void HandleResponse(
        const TActorContext& ctx,
        const typename TMethod::TResponse::TPtr& ev);

    bool HandleRequests(STFUNC_SIG);
    bool LogLateMessage(ui32 evType, const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TVolumeClientActor::TVolumeClientActor(
        TStorageConfigPtr config,
        ITraceSerializerPtr traceSerializer,
        const TActorId& sessionActorId,
        TString diskId,
        ui64 tabletId)
    : TActor(&TThis::StateWork)
    , TraceSerializer(std::move(traceSerializer))
    , SessionActorId(sessionActorId)
    , DiskId(std::move(diskId))
    , TabletId(tabletId)
    , ClientConfig(CreateTabletPipeClientConfig(*config))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TVolumeClientActor::OnConnectionError(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    LOG_WARN(ctx, TBlockStoreComponents::SERVICE,
         "Connection to volume with diskId %s (%lu) failed at %s: %s",
         DiskId.Quote().data(),
         TabletId,
         ToString(ctx.Now()).data(),
         FormatError(error).data());

    CancelActiveRequests(ctx);

    auto msg = std::make_unique<TEvServicePrivate::TEvVolumePipeReset>(GetCycleCount());
    NCloud::Send(ctx, SessionActorId, std::move(msg));
}

void TVolumeClientActor::CancelActiveRequests(const TActorContext& ctx)
{
    for (auto it = ActiveRequests.begin(); it != ActiveRequests.end(); ) {
        TAutoPtr<IEventHandle> handle(it->second.Request.release());
        Receive(handle, ctx);
        ActiveRequests.erase(it++);
    }
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeClientActor::HandleConnect(
    TEvTabletPipe::TEvClientConnected::TPtr& ev,
    const TActorContext& ctx)
{
    if (!PipeClient) {
        return;
    }

    const auto* msg = ev->Get();

    if (msg->TabletId != TabletId) {
        return;
    }

    if (!msg->ServerId) {
        auto error = MakeKikimrError(msg->Status, "Could not connect");
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Cannot connect to tablet %lu: %s",
            msg->TabletId,
            FormatError(error).data());
        PipeClient = {};

        CancelActiveRequests(ctx);
        return;
    }
}

void TVolumeClientActor::HandleDisconnect(
    TEvTabletPipe::TEvClientDestroyed::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    if (PipeClient) {
        PipeClient = {};

        auto error = MakeError(E_REJECTED, "Connection broken");
        OnConnectionError(ctx, error);
    }
}

void TVolumeClientActor::HandleResetPipeClient(
    TEvServicePrivate::TEvResetPipeClient::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
        "Got request to close pipe for tablet %lu",
        TabletId);

    NCloud::Send<TEvents::TEvPoisonPill>(ctx, PipeClient);
    PipeClient = {};

    CancelActiveRequests(ctx);
}

template <typename TMethod>
void TVolumeClientActor::HandleRequest(
    const TActorContext& ctx,
    const typename TMethod::TRequest::TPtr& ev)
{
    // ActiveRequests contains IEventHandles without embedded messages,
    // only information about event type and sender. When we detect failure
    // in the pipe, we re-send all IEventHandle's from ActiveRequests
    // to ourself, enabling standard path for message processing.
    // So we need to handle the cases when message is re-sent or just came from
    // outside. We have to to check if message buffer is present
    // otherwise it is safe to use Get() to retrieve actual message.
    if (!ev->HasBuffer() && !ev->HasEvent()) {
        auto response = std::make_unique<typename TMethod::TResponse>(
            MakeError(E_REJECTED, "Tablet is dead"));

        BLOCKSTORE_TRACE_SENT(ctx, &ev->TraceId, this, response);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    if (!PipeClient) {
        PipeClient = ctx.Register(CreateClient(SelfId(), TabletId, ClientConfig));
        ++Generation;
    }

    LOG_TRACE(ctx, TBlockStoreComponents::SERVICE,
        "Forward request to volume: %lu (remote: %s)",
        TabletId,
        ToString(PipeClient).data());

    ui64 requestId = ++RequestId;

    auto* msg = ev->Get();
    SetRequestGeneration(Generation, *msg);

    // so far we don't support client requests with more than 1 call context
    // except for the batched requests in partition. However in future we may
    // have batching for client requests.
    TraceSerializer->BuildTraceRequest(
        *msg->Record.MutableHeaders()->MutableInternal()->MutableTrace(),
        msg->CallContext->LWOrbit);

    auto event = std::make_unique<IEventHandle>(
        ev->Recipient,
        SelfId(),
        ev->ReleaseBase().Release(),
        0,          // flags
        requestId,  // cookie
        nullptr,    // forwardOnNondelivery
        ev->TraceId.Clone());

    ActiveRequests.emplace(
        requestId,
        TActiveRequest(
            IEventHandlePtr(ev.Release()),
            msg->CallContext,
            GetCycleCount()));

    LWTRACK(
        RequestSentPipe,
        msg->CallContext->LWOrbit,
        TMethod::Name,
        msg->CallContext->RequestId);

    event->Rewrite(TEvTabletPipe::EvSend, PipeClient);
    ctx.Send(event.release());
}

template <typename TMethod>
void TVolumeClientActor::HandleResponse(
    const TActorContext& ctx,
    const typename TMethod::TResponse::TPtr& ev)
{
    auto it = ActiveRequests.find(ev->Cookie);
    if (it == ActiveRequests.end()) {
        // ActiveRequests are cleared upon connection reset
        LogLateMessage(ev->GetTypeRewrite(), ctx);
        return;
    }

    auto* msg = ev->Get();

    if (it->second.CallContext->LWOrbit.HasShuttles()) {
        TraceSerializer->HandleTraceInfo(
            msg->Record.GetTrace(),
            it->second.CallContext->LWOrbit,
            it->second.SendTime,
            GetCycleCount());
        msg->Record.ClearTrace();
    }

    using TProtoType = decltype(TMethod::TResponse::Record);

    if constexpr (THasGetThrottlerDelay<TProtoType>::value) {
        it->second.CallContext->AddTime(
            EProcessingStage::Postponed,
            TDuration::MicroSeconds(msg->Record.GetThrottlerDelay()));
    }

    // forward response to the caller
    TAutoPtr<IEventHandle> event;
    if (ev->HasEvent()) {
        event = new IEventHandle(
            it->second.Request->Sender,
            ev->Sender,
            ev->ReleaseBase().Release(),
            ev->Flags,
            it->second.Request->Cookie,
            nullptr,    // undeliveredRequestActor
            std::move(ev->TraceId));
    } else {
        event = new IEventHandle(
            ev->Type,
            ev->Flags,
            it->second.Request->Sender,
            ev->Sender,
            ev->ReleaseChainBuffer(),
            it->second.Request->Cookie,
            nullptr,    // undeliveredRequestActor
            std::move(ev->TraceId));
    }

    ctx.Send(event);
    ActiveRequests.erase(it);
}

void TVolumeClientActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    CancelActiveRequests(ctx);
    NCloud::Send<TEvents::TEvPoisonPill>(ctx, PipeClient);
}

bool TVolumeClientActor::HandleRequests(STFUNC_SIG)
{
#define BLOCKSTORE_HANDLE_METHOD(name, ns)                                     \
    case ns::TEv##name##Request::EventType: {                                  \
        auto* x = reinterpret_cast<ns::TEv##name##Request::TPtr*>(&ev);        \
        HandleRequest<ns::T##name##Method>(ctx, *x);                           \
        break;                                                                 \
    }                                                                          \
    case ns::TEv##name##Response::EventType: {                                 \
        auto* x = reinterpret_cast<ns::TEv##name##Response::TPtr*>(&ev);       \
        HandleResponse<ns::T##name##Method>(ctx, *x);                          \
        break;                                                                 \
    }                                                                          \
// BLOCKSTORE_HANDLE_METHOD

    switch (ev->GetTypeRewrite()) {
        BLOCKSTORE_VOLUME_REQUESTS(BLOCKSTORE_HANDLE_METHOD, TEvVolume)
        BLOCKSTORE_VOLUME_REQUESTS_FWD_SERVICE(BLOCKSTORE_HANDLE_METHOD, TEvService)

        default:
            return false;
    }

    return true;

#undef BLOCKSTORE_HANDLE_METHOD
}

bool TVolumeClientActor::LogLateMessage(ui32 evType, const TActorContext& ctx)
{
#define BLOCKSTORE_LOG_MESSAGE(name, ns)                                       \
    case ns::TEv##name##Request::EventType: {                                  \
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,                         \
            "Late request : (0x%08X) %s request",                              \
            evType,                                                            \
            #name);                                                            \
        break;                                                                 \
    }                                                                          \
    case ns::TEv##name##Response::EventType: {                                 \
        LOG_WARN(ctx, TBlockStoreComponents::SERVICE,                         \
            "Late response : (0x%08X) %s response",                            \
            evType,                                                            \
            #name);                                                            \
        break;                                                                 \
    }                                                                          \
// BLOCKSTORE_LOG_MESSAGE

    switch (evType) {
        BLOCKSTORE_VOLUME_REQUESTS(BLOCKSTORE_LOG_MESSAGE, TEvVolume)
        BLOCKSTORE_VOLUME_REQUESTS_FWD_SERVICE(BLOCKSTORE_LOG_MESSAGE, TEvService)

        default:
            return false;
    }

    return true;

#undef BLOCKSTORE_LOG_MESSAGE
}


STFUNC(TVolumeClientActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvTabletPipe::TEvClientConnected, HandleConnect);
        HFunc(TEvTabletPipe::TEvClientDestroyed, HandleDisconnect);
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvServicePrivate::TEvResetPipeClient, HandleResetPipeClient);

        default:
            if (!HandleRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            }
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr CreateVolumeClient(
    TStorageConfigPtr config,
    ITraceSerializerPtr traceSerializer,
    const TActorId& sessionActorId,
    TString diskId,
    ui64 tabletId)
{
    return std::make_unique<TVolumeClientActor>(
        std::move(config),
        std::move(traceSerializer),
        sessionActorId,
        std::move(diskId),
        tabletId);
}

}   // namespace NCloud::NBlockStore::NStorage

