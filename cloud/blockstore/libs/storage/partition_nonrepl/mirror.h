#pragma once

#include "part_nonrepl_events_private.h"

#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

namespace NCloud::NBlockStore::NStorage {

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
class TWriteRequestActor final
    : public NActors::TActorBootstrapped<TWriteRequestActor<TMethod>>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TVector<NActors::TActorId> Partitions;
    const typename TMethod::TRequest::ProtoRecordType Request;
    const TString DiskId;
    const NActors::TActorId ParentActorId;
    const ui64 NonreplicatedRequestCounter;

    TVector<TCallContextPtr> ForkedCallContexts;
    ui32 Responses = 0;
    typename TMethod::TResponse::ProtoRecordType Record;

    using TBase = NActors::TActorBootstrapped<TWriteRequestActor<TMethod>>;

public:
    TWriteRequestActor(
        TRequestInfoPtr requestInfo,
        TVector<NActors::TActorId> partitions,
        typename TMethod::TRequest::ProtoRecordType request,
        TString diskId,
        NActors::TActorId parentActorId,
        ui64 nonreplicatedRequestCounter);

    void Bootstrap(const NActors::TActorContext& ctx);

private:
    void SendRequests(const NActors::TActorContext& ctx);
    bool HandleError(const NActors::TActorContext& ctx, NProto::TError error);
    void Done(const NActors::TActorContext& ctx);

private:
    STFUNC(StateWork);

    void HandleResponse(
        const typename TMethod::TResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandlePoisonPill(
        const NActors::TEvents::TEvPoisonPill::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleUndelivery(
        const typename TMethod::TRequest::TPtr& ev,
        const NActors::TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
TWriteRequestActor<TMethod>::TWriteRequestActor(
        TRequestInfoPtr requestInfo,
        TVector<NActors::TActorId> partitions,
        typename TMethod::TRequest::ProtoRecordType request,
        TString diskId,
        NActors::TActorId parentActorId,
        ui64 nonreplicatedRequestCounter)
    : RequestInfo(std::move(requestInfo))
    , Partitions(std::move(partitions))
    , Request(std::move(request))
    , DiskId(std::move(diskId))
    , ParentActorId(parentActorId)
    , NonreplicatedRequestCounter(nonreplicatedRequestCounter)
{
    TBase::ActivityType = TBlockStoreActivities::PARTITION_WORKER;
}

template <typename TMethod>
void TWriteRequestActor<TMethod>::Bootstrap(const NActors::TActorContext& ctx)
{
    TRequestScope timer(*RequestInfo);

    TBase::Become(&TBase::TThis::StateWork);

    LWTRACK(
        RequestReceived_PartitionWorker,
        RequestInfo->CallContext->LWOrbit,
        TMethod::Name,
        RequestInfo->CallContext->RequestId);

    SendRequests(ctx);
}

template <typename TMethod>
void TWriteRequestActor<TMethod>::SendRequests(const NActors::TActorContext& ctx)
{
    for (const auto& actorId: Partitions) {
        auto request = std::make_unique<typename TMethod::TRequest>();
        auto& callContext = *RequestInfo->CallContext;
        if (!callContext.LWOrbit.Fork(request->CallContext->LWOrbit)) {
            LWTRACK(
                ForkFailed,
                callContext.LWOrbit,
                TMethod::Name,
                callContext.RequestId);
        }
        ForkedCallContexts.push_back(request->CallContext);
        request->Record = Request;

        auto traceId = RequestInfo->TraceId.Clone();
        BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

        TAutoPtr<NActors::IEventHandle> event(
            new NActors::IEventHandle(
                actorId,
                ctx.SelfID,
                request.get(),
                NActors::IEventHandle::FlagForwardOnNondelivery,
                0,  // cookie
                &ctx.SelfID,    // forwardOnNondelivery
                std::move(traceId)
            )
        );
        request.release();

        ctx.Send(event);
    }
}

template <typename TMethod>
void TWriteRequestActor<TMethod>::Done(const NActors::TActorContext& ctx)
{
    auto response = std::make_unique<typename TMethod::TResponse>();
    response->Record = std::move(Record);

    auto& callContext = *RequestInfo->CallContext;
    for (auto& cc: ForkedCallContexts) {
        callContext.LWOrbit.Join(cc->LWOrbit);
    }

    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);

    LWTRACK(
        ResponseSent_PartitionWorker,
        RequestInfo->CallContext->LWOrbit,
        TMethod::Name,
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));

    using TCompletion =
        TEvNonreplPartitionPrivate::TEvWriteOrZeroCompleted;
    auto completion =
        std::make_unique<TCompletion>(NonreplicatedRequestCounter);

    NCloud::Send(ctx, ParentActorId, std::move(completion));

    TBase::Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TWriteRequestActor<TMethod>::HandleUndelivery(
    const typename TMethod::TRequest::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    Y_UNUSED(ev);

    LOG_WARN(ctx, TBlockStoreComponents::PARTITION_WORKER,
        "[%s] %s request undelivered to some nonrepl partitions",
        DiskId.c_str(),
        TMethod::Name);

    Record.MutableError()->CopyFrom(MakeError(E_REJECTED, TStringBuilder()
        << TMethod::Name << " request undelivered to some nonrepl partitions"));

    if (++Responses < Partitions.size()) {
        return;
    }

    Done(ctx);
}

template <typename TMethod>
void TWriteRequestActor<TMethod>::HandleResponse(
    const typename TMethod::TResponse::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    if (HasError(msg->Record)) {
        LOG_ERROR(ctx, TBlockStoreComponents::PARTITION_WORKER,
            "[%s] %s got error from nonreplicated partition: %s",
            DiskId.c_str(),
            TMethod::Name,
            FormatError(msg->Record.GetError()).c_str());
    }

    if (!HasError(Record)) {
        Record = std::move(msg->Record);
    }

    if (++Responses < Partitions.size()) {
        return;
    }

    Done(ctx);
}

template <typename TMethod>
void TWriteRequestActor<TMethod>::HandlePoisonPill(
    const NActors::TEvents::TEvPoisonPill::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    Y_UNUSED(ev);

    Record.MutableError()->CopyFrom(MakeError(E_REJECTED, "Dead"));
    Done(ctx);
}

template <typename TMethod>
STFUNC(TWriteRequestActor<TMethod>::StateWork)
{
    TRequestScope timer(*RequestInfo);

    switch (ev->GetTypeRewrite()) {
        HFunc(NActors::TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TMethod::TResponse, HandleResponse);
        HFunc(TMethod::TRequest, HandleUndelivery);

        default:
            HandleUnexpectedEvent(
                ctx,
                ev,
                TBlockStoreComponents::PARTITION_WORKER);
            break;
    }
}

}   // namespace NCloud::NBlockStore::NStorage
