#include "volume_actor.h"

#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/api/partition.h>
#include <cloud/blockstore/libs/storage/api/undelivered.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <cloud/storage/core/libs/diagnostics/trace_serializer.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;
using namespace NPartition;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TPartitionDescr
{
    ui64 TabletId = 0;
    TActorId ActorId;
};

using TPartitionDescrs = TVector<TPartitionDescr>;

////////////////////////////////////////////////////////////////////////////////

using TCreateMethod = TEvService::TCreateCheckpointMethod;
using TDeleteMethod = TEvService::TDeleteCheckpointMethod;
using TDeleteDataMethod = TEvVolume::TDeleteCheckpointDataMethod;

const char* GetCheckpointRequestName(ECheckpointRequestType reqType) {
    switch (reqType) {
        case ECheckpointRequestType::Delete:
            return TDeleteMethod::Name;
        case ECheckpointRequestType::DeleteData:
            return TDeleteDataMethod::Name;
        case ECheckpointRequestType::Create:
            return TCreateMethod::Name;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
class TCheckpointActor final
    : public TActorBootstrapped<TCheckpointActor<TMethod>>
{
private:
    using TThis = TCheckpointActor<TMethod>;
    using TBase = TActor<TThis>;

private:
    const TRequestInfoPtr RequestInfo;
    const ui64 RequestId;
    const TString CheckpointId;
    const ui64 VolumeTabletId;
    const TActorId VolumeActorId;
    const TPartitionDescrs PartitionDescrs;
    const TRequestTraceInfo TraceInfo;

    ui32 DrainResponses = 0;
    ui32 Responses = 0;
    NProto::TError Error;

    TVector<TCallContextPtr> ChildCallContexts;

public:
    TCheckpointActor(
        TRequestInfoPtr requestInfo,
        ui64 requestId,
        TString checkpointId,
        ui64 volumeTabletId,
        TActorId volumeActorId,
        TPartitionDescrs partitionDescrs,
        TRequestTraceInfo traceInfo);

    void Bootstrap(const TActorContext& ctx);
    void Drain(const TActorContext& ctx);
    void DoAction(const TActorContext& ctx);
    void MarkCheckpointRequestCompleted(const TActorContext& ctx);
    void ReplyAndDie(const TActorContext& ctx);

private:
    void ForkTraces(TCallContextPtr callContext);
    void JoinTraces(ui32 cookie);

private:
    STFUNC(StateDrain);
    STFUNC(StateDoAction);
    STFUNC(StateMarkCheckpointRequestCompleted);

    void HandleResponse(
        const typename TMethod::TResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleUndelivery(
        const typename TMethod::TRequest::TPtr& ev,
        const TActorContext& ctx);

    void HandleDrainResponse(
        const TEvPartition::TEvDrainResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleDrainUndelivery(
        const TEvPartition::TEvDrainRequest::TPtr& ev,
        const TActorContext& ctx);

    void HandleMarkCheckpointRequestCompletedResponse(
        const TEvVolumePrivate::TEvMarkCheckpointRequestCompletedResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
TCheckpointActor<TMethod>::TCheckpointActor(
        TRequestInfoPtr requestInfo,
        ui64 requestId,
        TString checkpointId,
        ui64 volumeTabletId,
        TActorId volumeActorId,
        TPartitionDescrs partitionDescrs,
        TRequestTraceInfo traceInfo)
    : RequestInfo(std::move(requestInfo))
    , RequestId(requestId)
    , CheckpointId(std::move(checkpointId))
    , VolumeTabletId(volumeTabletId)
    , VolumeActorId(volumeActorId)
    , PartitionDescrs(std::move(partitionDescrs))
    , TraceInfo(std::move(traceInfo))
    , ChildCallContexts(Reserve(PartitionDescrs.size()))
{
    TBase::ActivityType = TBlockStoreActivities::VOLUME;
}

template <typename TMethod>
void TCheckpointActor<TMethod>::Drain(const TActorContext& ctx)
{
    ui32 cookie = 0;
    for (const auto& x: PartitionDescrs) {
        LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
            "[%lu] Sending Drain request to partition %lu",
            VolumeTabletId,
            x.TabletId);

        const auto selfId = TBase::SelfId();
        auto request = std::make_unique<TEvPartition::TEvDrainRequest>();

        ForkTraces(request->CallContext);

        auto event = std::make_unique<IEventHandle>(
            x.ActorId,
            selfId,
            request.get(),
            IEventHandle::FlagForwardOnNondelivery,
            cookie++,
            &selfId,
            RequestInfo->TraceId.Clone()
        );
        request.release();

        ctx.Send(event.release());
    }

    TBase::Become(&TThis::StateDrain);
}

template <typename TMethod>
void TCheckpointActor<TMethod>::MarkCheckpointRequestCompleted(const TActorContext& ctx)
{
    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Sending MarkCheckpointRequestCompleted to volume",
        VolumeTabletId);

    const auto selfId = TBase::SelfId();

    auto requestInfo = CreateRequestInfo(
        selfId,
        0,  // cookie
        RequestInfo->CallContext,
        RequestInfo->TraceId.Clone()
    );

    auto request = std::make_unique<TEvVolumePrivate::TEvMarkCheckpointRequestCompletedRequest>(
        std::move(requestInfo),
        RequestId
    );

    auto event = std::make_unique<IEventHandle>(
        VolumeActorId,
        selfId,
        request.get(),
        IEventHandle::FlagForwardOnNondelivery,
        0,  // cookie
        &selfId,
        RequestInfo->TraceId.Clone()
    );

    request.release();

    ctx.Send(event.release());

    TBase::Become(&TThis::StateMarkCheckpointRequestCompleted);
}

template <typename TMethod>
void TCheckpointActor<TMethod>::ReplyAndDie(const TActorContext& ctx)
{
    auto response = std::make_unique<typename TMethod::TResponse>(Error);

    if (TraceInfo.IsTraced) {
        TraceInfo.TraceSerializer->BuildTraceInfo(
            *response->Record.MutableTrace(),
            RequestInfo->CallContext->LWOrbit,
            TraceInfo.ReceiveTime,
            GetCycleCount());
    }

    LWTRACK(
        ResponseSent_Volume,
        RequestInfo->CallContext->LWOrbit,
        TMethod::Name,
        RequestInfo->CallContext->RequestId);

    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);
    NCloud::Reply(ctx, *RequestInfo, std::move(response));

    TBase::Die(ctx);
}

template <typename TMethod>
void TCheckpointActor<TMethod>::ForkTraces(TCallContextPtr callContext)
{
    auto& cc = RequestInfo->CallContext;
    if (!cc->LWOrbit.Fork(callContext->LWOrbit)) {
        LWTRACK(ForkFailed, cc->LWOrbit, TMethod::Name, cc->RequestId);
    }

    ChildCallContexts.push_back(callContext);
}

template <typename TMethod>
void TCheckpointActor<TMethod>::JoinTraces(ui32 cookie)
{
    if (cookie < ChildCallContexts.size()) {
        if (ChildCallContexts[cookie]) {
            auto& cc = RequestInfo->CallContext;
            cc->LWOrbit.Join(ChildCallContexts[cookie]->LWOrbit);
            ChildCallContexts[cookie].Reset();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TCheckpointActor<TMethod>::HandleDrainResponse(
    const TEvPartition::TEvDrainResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    JoinTraces(ev->Cookie);

    if (FAILED(msg->GetStatus())) {
        Error = msg->GetError();

        NCloud::Send(
            ctx,
            VolumeActorId,
            std::make_unique<TEvents::TEvPoisonPill>()
        );

        ReplyAndDie(ctx);
        return;
    }

    if (++DrainResponses == PartitionDescrs.size()) {
        ChildCallContexts.clear();
        DoAction(ctx);
    }
}

template <typename TMethod>
void TCheckpointActor<TMethod>::HandleDrainUndelivery(
    const TEvPartition::TEvDrainRequest::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    Error = MakeError(
        E_REJECTED,
        "failed to deliver drain request to some partitions"
    );

    NCloud::Send(
        ctx,
        VolumeActorId,
        std::make_unique<TEvents::TEvPoisonPill>()
    );

    ReplyAndDie(ctx);
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TCheckpointActor<TMethod>::HandleResponse(
    const typename TMethod::TResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    JoinTraces(ev->Cookie);

    if (FAILED(msg->GetStatus())) {
        Error = msg->GetError();

        NCloud::Send(
            ctx,
            VolumeActorId,
            std::make_unique<TEvents::TEvPoisonPill>()
        );

        ReplyAndDie(ctx);
        return;
    }

    if (++Responses == PartitionDescrs.size()) {
        MarkCheckpointRequestCompleted(ctx);
    }
}

template <typename TMethod>
void TCheckpointActor<TMethod>::HandleUndelivery(
    const typename TMethod::TRequest::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    Error = MakeError(
        E_REJECTED,
        "failed to deliver checkpoint request to some partitions"
    );

    NCloud::Send(
        ctx,
        VolumeActorId,
        std::make_unique<TEvents::TEvPoisonPill>()
    );

    ReplyAndDie(ctx);
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TCheckpointActor<TMethod>::HandleMarkCheckpointRequestCompletedResponse(
    const TEvVolumePrivate::TEvMarkCheckpointRequestCompletedResponse::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    ReplyAndDie(ctx);
}

template <typename TMethod>
void TCheckpointActor<TMethod>::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    Error = MakeError(
        E_REJECTED,
        "tablet is dead"
    );

    ReplyAndDie(ctx);
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
STFUNC(TCheckpointActor<TMethod>::StateDrain)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvPartition::TEvDrainResponse, HandleDrainResponse);
        HFunc(TEvPartition::TEvDrainRequest, HandleDrainUndelivery);
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME);
            break;
    }
}

template <typename TMethod>
STFUNC(TCheckpointActor<TMethod>::StateDoAction)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TMethod::TResponse, HandleResponse);
        HFunc(TMethod::TRequest, HandleUndelivery);
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME);
            break;
    }
}

template <typename TMethod>
STFUNC(TCheckpointActor<TMethod>::StateMarkCheckpointRequestCompleted)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(
            TEvVolumePrivate::TEvMarkCheckpointRequestCompletedResponse,
            HandleMarkCheckpointRequestCompletedResponse
        );
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TCheckpointActor<TMethod>::DoAction(const TActorContext& ctx)
{
    ui32 cookie = 0;
    for (const auto& x: PartitionDescrs) {
        LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
            "[%lu] Sending %s request to partition %lu",
            VolumeTabletId,
            TMethod::Name,
            x.TabletId);

        const auto selfId = TBase::SelfId();
        auto request = std::make_unique<typename TMethod::TRequest>();
        request->Record.SetCheckpointId(CheckpointId);

        ForkTraces(request->CallContext);

        auto event = std::make_unique<IEventHandle>(
            x.ActorId,
            selfId,
            request.get(),
            IEventHandle::FlagForwardOnNondelivery,
            cookie++,
            &selfId,
            RequestInfo->TraceId.Clone()
        );
        request.release();

        ctx.Send(event.release());
    }

    TBase::Become(&TThis::StateDoAction);
}

template <typename TMethod>
void TCheckpointActor<TMethod>::Bootstrap(const TActorContext& ctx)
{
    DoAction(ctx);
}

template<>
void TCheckpointActor<TCreateMethod>::Bootstrap(const TActorContext& ctx)
{
    // if this is a single partion disk
    // then skip drain stuff
    if (PartitionDescrs.size() == 1) {
        DoAction(ctx);
    } else {
        Drain(ctx);
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

template <>
bool TVolumeActor::HandleRequest<TCreateMethod>(
    const TActorContext& ctx,
    const TCreateMethod::TRequest::TPtr& ev,
    bool isTraced,
    ui64 traceTs)
{
    Y_VERIFY_DEBUG(!State->GetNonreplicatedPartitionActor());

    auto requestInfo = CreateRequestInfo<TCreateMethod>(
        ev->Sender,
        ev->Cookie,
        ev->Get()->CallContext,
        std::move(ev->TraceId));

    AddTransaction(*requestInfo);

    ExecuteTx<TSaveCheckpointRequest>(
        ctx,
        std::move(requestInfo),
        State->GenerateCheckpointRequestId(),
        ev->Get()->Record.GetCheckpointId(),
        ECheckpointRequestType::Create,
        isTraced,
        traceTs
    );

    return true;
}

template <>
bool TVolumeActor::HandleRequest<TDeleteMethod>(
    const TActorContext& ctx,
    const TDeleteMethod::TRequest::TPtr& ev,
    bool isTraced,
    ui64 traceTs)
{
    Y_VERIFY_DEBUG(!State->GetNonreplicatedPartitionActor());

    auto requestInfo = CreateRequestInfo<TDeleteMethod>(
        ev->Sender,
        ev->Cookie,
        ev->Get()->CallContext,
        std::move(ev->TraceId));

    AddTransaction(*requestInfo);

    ExecuteTx<TSaveCheckpointRequest>(
        ctx,
        std::move(requestInfo),
        State->GenerateCheckpointRequestId(),
        ev->Get()->Record.GetCheckpointId(),
        ECheckpointRequestType::Delete,
        isTraced,
        traceTs
    );

    return true;
}

template <>
bool TVolumeActor::HandleRequest<TDeleteDataMethod>(
    const TActorContext& ctx,
    const TDeleteDataMethod::TRequest::TPtr& ev,
    bool isTraced,
    ui64 traceTs)
{
    Y_VERIFY_DEBUG(!State->GetNonreplicatedPartitionActor());

    auto requestInfo = CreateRequestInfo<TDeleteDataMethod>(
        ev->Sender,
        ev->Cookie,
        ev->Get()->CallContext,
        std::move(ev->TraceId));

    AddTransaction(*requestInfo);

    ExecuteTx<TSaveCheckpointRequest>(
        ctx,
        std::move(requestInfo),
        State->GenerateCheckpointRequestId(),
        ev->Get()->Record.GetCheckpointId(),
        ECheckpointRequestType::DeleteData,
        isTraced,
        traceTs
    );

    return true;
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleMarkCheckpointRequestCompleted(
    const TEvVolumePrivate::TEvMarkCheckpointRequestCompletedRequest::TPtr& ev,
    const TActorContext& ctx)
{
    ExecuteTx<TMarkCheckpointRequestCompleted>(
        ctx,
        std::move(ev->Get()->RequestInfo),
        ev->Get()->RequestId
    );
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::ProcessNextCheckpointRequest(const TActorContext& ctx)
{
    // only one simultaneous checkpoint request is supported, other requests
    // should wait
    if (CheckpointRequestInProgress) {
        return;
    }

    // there is no FIFO guarantee for requests sent via TPartitionRequestActor
    // and requests forwarded via TVolumeActor => we can start checkpoint
    // creation only if there are no TPartitionRequestActor-based requests
    // in flight currently
    if (MultipartitionWriteAndZeroRequestsInProgress) {
        return;
    }

    // nothing to do
    if (CheckpointRequestQueue.empty()) {
        return;
    }

    CheckpointRequestInProgress = true;

    auto& request = CheckpointRequestQueue.front();

    TPartitionDescrs partitionDescrs;
    for (const auto& p: State->GetPartitions()) {
        partitionDescrs.push_back({p.TabletId, p.Owner});

        LOG_TRACE(ctx, TBlockStoreComponents::VOLUME,
            "[%lu] Forward %s request to partition: %lu %s",
            TabletID(),
            GetCheckpointRequestName(request.ReqType),
            p.TabletId,
            ToString(p.Owner).data()
        );
    }

    TActorId actorId;
    switch (request.ReqType) {
        case ECheckpointRequestType::Create: {
            actorId = NCloud::Register<TCheckpointActor<TCreateMethod>>(
                ctx,
                request.RequestInfo,
                request.RequestId,
                request.CheckpointId,
                TabletID(),
                SelfId(),
                std::move(partitionDescrs),
                TRequestTraceInfo(request.IsTraced, request.TraceTs, TraceSerializer)
            );
            break;
        }
        case ECheckpointRequestType::Delete: {
            actorId = NCloud::Register<TCheckpointActor<TDeleteMethod>>(
                ctx,
                request.RequestInfo,
                request.RequestId,
                request.CheckpointId,
                TabletID(),
                SelfId(),
                std::move(partitionDescrs),
                TRequestTraceInfo(request.IsTraced, request.TraceTs, TraceSerializer)
            );
            break;
        }
        case ECheckpointRequestType::DeleteData: {
            actorId = NCloud::Register<TCheckpointActor<TDeleteDataMethod>>(
                ctx,
                request.RequestInfo,
                request.RequestId,
                request.CheckpointId,
                TabletID(),
                SelfId(),
                std::move(partitionDescrs),
                TRequestTraceInfo(request.IsTraced, request.TraceTs, TraceSerializer)
            );
            break;
        }
        default:
            Y_VERIFY(0);
    }

    Actors.insert(actorId);
}

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::PrepareSaveCheckpointRequest(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TSaveCheckpointRequest& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TVolumeActor::ExecuteSaveCheckpointRequest(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TSaveCheckpointRequest& args)
{
    Y_UNUSED(ctx);

    TVolumeDatabase db(tx.DB);

    auto ts = ctx.Now();
    if (State->GetCheckpointRequests()) {
        const auto& lastReq = State->GetCheckpointRequests().back();
        ts = Max(ts, lastReq.Timestamp + TDuration::MicroSeconds(1));
    }

    db.WriteCheckpointRequest(args.RequestId, args.CheckpointId, ts, args.ReqType);
    State->GetCheckpointRequests().emplace_back(
        args.RequestId,
        args.CheckpointId,
        ts,
        args.ReqType,
        ECheckpointRequestState::New);
}

void TVolumeActor::CompleteSaveCheckpointRequest(
    const TActorContext& ctx,
    TTxVolume::TSaveCheckpointRequest& args)
{
    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] CheckpointRequest %lu %s saved",
        TabletID(),
        args.RequestId,
        args.CheckpointId.Quote().c_str());

    CheckpointRequestQueue.emplace_back(
        args.RequestInfo,
        args.RequestId,
        args.CheckpointId,
        args.ReqType,
        args.IsTraced,
        args.TraceTs
    );

    if (CheckpointRequestQueue.size() > 1) {
        return;
    }
    RemoveTransaction(*args.RequestInfo);
    ProcessNextCheckpointRequest(ctx);
}

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::PrepareMarkCheckpointRequestCompleted(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TMarkCheckpointRequestCompleted& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TVolumeActor::ExecuteMarkCheckpointRequestCompleted(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TMarkCheckpointRequestCompleted& args)
{
    Y_UNUSED(ctx);

    TVolumeDatabase db(tx.DB);
    db.MarkCheckpointRequestCompleted(args.RequestId);
}

void TVolumeActor::CompleteMarkCheckpointRequestCompleted(
    const TActorContext& ctx,
    TTxVolume::TMarkCheckpointRequestCompleted& args)
{
    CheckpointRequestInProgress = false;
    for (auto it = State->GetCheckpointRequests().rbegin();
            it != State->GetCheckpointRequests().rend(); ++it)
    {
        if (it->RequestId == args.RequestId) {
            LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
                "[%lu] CheckpointRequest %lu %s marked completed",
                TabletID(),
                args.RequestId,
                it->CheckpointId.Quote().c_str());

            it->State = ECheckpointRequestState::Completed;
            break;
        }
    }

    auto& currentRequest = CheckpointRequestQueue.front();
    switch (currentRequest.ReqType) {
        case ECheckpointRequestType::Create: {
            State->GetActiveCheckpoints().Add(currentRequest.CheckpointId);
            break;
        }
        case ECheckpointRequestType::Delete: {
            State->GetActiveCheckpoints().Delete(currentRequest.CheckpointId);
            break;
        }
        case ECheckpointRequestType::DeleteData: {
            State->GetActiveCheckpoints().DeleteData(currentRequest.CheckpointId);
            break;
        }
        default:
            Y_VERIFY(0);
    }

    CheckpointRequestQueue.pop_front();

    NCloud::Reply(
        ctx,
        *args.RequestInfo,
        std::make_unique<TEvVolumePrivate::TEvMarkCheckpointRequestCompletedResponse>()
    );
    Actors.erase(args.RequestInfo->Sender);

    ProcessNextCheckpointRequest(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage
