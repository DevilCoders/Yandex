#include "volume_actor.h"

#include "partition_requests.h"

#include <cloud/blockstore/libs/common/proto_helpers.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/api/undelivered.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/forward_helpers.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/volume/model/merge.h>
#include <cloud/blockstore/libs/storage/volume/model/stripe.h>

#include <cloud/storage/core/libs/common/media.h>
#include <cloud/storage/core/libs/diagnostics/trace_serializer.h>

#include <util/generic/guid.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
TBlockRange64 BuildRequestBlockRange(const T&, const ui32)
{
    Y_VERIFY(0);
}

TBlockRange64 BuildRequestBlockRange(
    const TEvService::TEvWriteBlocksRequest& request,
    const ui32 blockSize)
{
    ui64 totalSize = CalculateBytesCount(request.Record, blockSize);
    Y_VERIFY(totalSize % blockSize == 0);

    return TBlockRange64::WithLength(
        request.Record.GetStartIndex(),
        totalSize / blockSize
    );
}

TBlockRange64 BuildRequestBlockRange(
    const TEvService::TEvWriteBlocksLocalRequest& request,
    const ui32 blockSize)
{
    Y_UNUSED(blockSize);

    return TBlockRange64::WithLength(
        request.Record.GetStartIndex(),
        request.Record.BlocksCount
    );
}

TBlockRange64 BuildRequestBlockRange(
    const TEvService::TEvZeroBlocksRequest& request,
    const ui32 blockSize)
{
    Y_UNUSED(blockSize);

    return TBlockRange64::WithLength(
        request.Record.GetStartIndex(),
        request.Record.GetBlocksCount()
    );
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void ChangeSessionId(T&, const TString&)
{
}

void ChangeSessionId(
    TEvService::TEvReadBlocksRequest& request,
    const TString& newSessionId)
{
    request.Record.SetSessionId(newSessionId);
}

void ChangeSessionId(
    TEvService::TEvWriteBlocksRequest& request,
    const TString& newSessionId)
{
    request.Record.SetSessionId(newSessionId);
}

void ChangeSessionId(
    TEvService::TEvZeroBlocksRequest& request,
    const TString& newSessionId)
{
    request.Record.SetSessionId(newSessionId);
}

void ChangeSessionId(
    TEvService::TEvReadBlocksLocalRequest& request,
    const TString& newSessionId)
{
    request.Record.SetSessionId(newSessionId);
}

void ChangeSessionId(
    TEvService::TEvWriteBlocksLocalRequest& request,
    const TString& newSessionId)
{
    request.Record.SetSessionId(newSessionId);
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
bool CanForwardToPartition(ui32 partitionCount)
{
    return partitionCount <= 1;
}

template <>
bool CanForwardToPartition<TEvService::TCreateCheckpointMethod>(ui32 partitionCount)
{
    Y_UNUSED(partitionCount);
    return false;
}

template <>
bool CanForwardToPartition<TEvService::TDeleteCheckpointMethod>(ui32 partitionCount)
{
    Y_UNUSED(partitionCount);
    return false;
}

template <>
bool CanForwardToPartition<TEvVolume::TDeleteCheckpointDataMethod>(ui32 partitionCount)
{
    Y_UNUSED(partitionCount);
    return false;
}

////////////////////////////////////////////////////////////////////////////////

Y_HAS_MEMBER(SetThrottlerDelay);

template <typename TResponse>
void StoreThrottlerDelay(TResponse& response, TDuration delay)
{
    using TProtoType = decltype(TResponse::Record);

    if constexpr (THasSetThrottlerDelay<TProtoType>::value) {
        response.Record.SetThrottlerDelay(delay.MicroSeconds());
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void RejectVolumeRequest(
    const TActorContext& ctx,
    IEventHandle& request,
    TCallContext& callContext,
    NProto::TError error)
{
    auto response = std::make_unique<typename TMethod::TResponse>(std::move(error));

    StoreThrottlerDelay(
        *response,
        callContext.Time(EProcessingStage::Postponed));

    NCloud::Reply(ctx, request, std::move(response));
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
bool TVolumeActor::HandleRequest(
    const TActorContext& ctx,
    const typename TMethod::TRequest::TPtr& ev,
    bool isTraced,
    ui64 traceTs)
{
    Y_VERIFY(!State->GetNonreplicatedPartitionActor());

    const auto blocksPerStripe =
        State->GetMeta().GetVolumeConfig().GetBlocksPerStripe();
    Y_VERIFY(blocksPerStripe);

    TVector<TPartitionRequest<TMethod>> partitionRequests;
    TBlockRange64 blockRange;

    bool ok = ToPartitionRequests<TMethod>(
        State->GetPartitions(),
        State->GetBlockSize(),
        blocksPerStripe,
        ev,
        &partitionRequests,
        &blockRange
    );

    if (!ok) {
        return false;
    }

    if (partitionRequests.size() == 1) {
        ev->Get()->Record = std::move(partitionRequests.front().Event->Record);
        SendRequestToPartition<TMethod>(
            ctx,
            ev,
            partitionRequests.front().PartitionId,
            traceTs
        );

        return true;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        ev->Get()->CallContext,
        std::move(ev->TraceId));

    for (const auto& partitionRequest: partitionRequests) {
        LOG_TRACE(ctx, TBlockStoreComponents::VOLUME,
            "[%lu] Forward %s request to partition: %lu %s",
            TabletID(),
            TMethod::Name,
            partitionRequest.TabletId,
            ToString(partitionRequest.ActorId).data()
        );
    }

    if (RequiresReadWriteAccess<TMethod>()) {
        ++MultipartitionWriteAndZeroRequestsInProgress;
    }

    NCloud::Register<TPartitionRequestActor<TMethod>>(
        ctx,
        std::move(requestInfo),
        SelfId(),
        VolumeRequestId,
        blockRange,
        blocksPerStripe,
        State->GetPartitions().size(),
        std::move(partitionRequests),
        TRequestTraceInfo(isTraced, traceTs, TraceSerializer)
    );

    return true;
}

template<>
bool TVolumeActor::HandleRequest<TEvService::TCreateCheckpointMethod>(
    const TActorContext& ctx,
    const TEvService::TCreateCheckpointMethod::TRequest::TPtr& ev,
    bool isTraced,
    ui64 traceTs);

template<>
bool TVolumeActor::HandleRequest<TEvService::TDeleteCheckpointMethod>(
    const TActorContext& ctx,
    const TEvService::TDeleteCheckpointMethod::TRequest::TPtr& ev,
    bool isTraced,
    ui64 traceTs);

template<>
bool TVolumeActor::HandleRequest<TEvVolume::TDeleteCheckpointDataMethod>(
    const TActorContext& ctx,
    const TEvVolume::TDeleteCheckpointDataMethod::TRequest::TPtr& ev,
    bool isTraced,
    ui64 traceTs);

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TVolumeActor::SendRequestToPartition(
    const TActorContext& ctx,
    const typename TMethod::TRequest::TPtr& ev,
    ui32 partitionId,
    ui64 traceTime)
{
    const auto& partition = State->GetPartitions()[partitionId];

    // forward request directly to the partition
    LOG_TRACE(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Forward %s request to partition: %lu %s",
        TabletID(),
        TMethod::Name,
        partition.TabletId,
        ToString(partition.Owner).data());

    const bool processed = SendRequestToPartitionWithUsedBlockTracking<TMethod>(
        ctx,
        ev,
        partition.Owner);

    if (processed) {
        return;
    }

    auto* msg = ev->Get();

    auto selfId = SelfId();
    auto callContext = msg->CallContext;
    msg->CallContext = MakeIntrusive<TCallContext>(callContext->RequestId);

    if (!callContext->LWOrbit.Fork(msg->CallContext->LWOrbit)) {
        LWTRACK(ForkFailed, callContext->LWOrbit, TMethod::Name, callContext->RequestId);
    }

    auto event = std::make_unique<IEventHandle>(
        partition.Owner,
        selfId,
        ev->ReleaseBase().Release(),
        IEventHandle::FlagForwardOnNondelivery, // flags
        VolumeRequestId,                        // cookie
        &selfId,                                // forwardOnNondelivery
        ev->TraceId.Clone());

    VolumeRequests.emplace(
        VolumeRequestId,
        TVolumeRequest(
            IEventHandlePtr(ev.Release()),
            std::move(callContext),
            msg->CallContext,
            traceTime,
            &RejectVolumeRequest<TMethod>));

    ctx.Send(event.release());
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TVolumeActor::FillResponse(
    typename TMethod::TResponse& response,
    TCallContext& callContext,
    ui64 startTime)
{
    LWTRACK(
        ResponseSent_Volume,
        callContext.LWOrbit,
        TMethod::Name,
        callContext.RequestId);

    if (TraceSerializer->IsTraced(callContext.LWOrbit)) {
        TraceSerializer->BuildTraceInfo(
            *response.Record.MutableTrace(),
            callContext.LWOrbit,
            startTime,
            GetCycleCount());
    }

    StoreThrottlerDelay(
        response,
        callContext.Time(EProcessingStage::Postponed));
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TVolumeActor::HandleResponse(
    const NActors::TActorContext& ctx,
    const typename TMethod::TResponse::TPtr& ev)
{
    Y_UNUSED(ctx);

    auto* msg = ev->Get();

    WriteAndZeroRequestsInFlight.RemoveRequest(ev->Cookie);

    if (auto it = VolumeRequests.find(ev->Cookie); it != VolumeRequests.end()) {
        auto& cc = *it->second.CallContext;
        cc.LWOrbit.Join(it->second.ForkedContext->LWOrbit);

        FillResponse<TMethod>(*msg, cc, it->second.ReceiveTime);

        // forward response to the caller
        TAutoPtr<IEventHandle> event = new IEventHandle(
            it->second.Request->Sender,
            ev->Sender,
            ev->ReleaseBase().Release(),
            ev->Flags,
            it->second.Request->Cookie,
            nullptr,    // undeliveredRequestActor
            std::move(ev->TraceId)
        );

        ctx.Send(event);

        VolumeRequests.erase(it);
    }
}

void TVolumeActor::HandleMultipartitionWriteOrZeroCompleted(
    const TEvVolumePrivate::TEvMultipartitionWriteOrZeroCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    WriteAndZeroRequestsInFlight.RemoveRequest(msg->VolumeRequestId);

    Y_VERIFY_DEBUG(MultipartitionWriteAndZeroRequestsInProgress > 0);
    --MultipartitionWriteAndZeroRequestsInProgress;
    ProcessNextCheckpointRequest(ctx);
}

void TVolumeActor::HandleWriteOrZeroCompleted(
    const TEvVolumePrivate::TEvWriteOrZeroCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ctx);

    auto* msg = ev->Get();

    WriteAndZeroRequestsInFlight.RemoveRequest(msg->VolumeRequestId);
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TVolumeActor::ForwardRequest(
    const TActorContext& ctx,
    const typename TMethod::TRequest::TPtr& ev)
{
    // PartitionRequests undelivery handing
    if (ev->Sender == SelfId()) {
        auto it = VolumeRequests.find(ev->Cookie);
        if (it != VolumeRequests.end()) {
            auto response = std::make_unique<typename TMethod::TResponse>(
                MakeError(E_REJECTED, TStringBuilder()
                    << "Volume not ready: " << State->GetDiskId().Quote()));

            FillResponse<TMethod>(*response, *it->second.CallContext, it->second.ReceiveTime);

            NCloud::Reply(ctx, *it->second.Request, std::move(response));

            WriteAndZeroRequestsInFlight.RemoveRequest(ev->Cookie);
            VolumeRequests.erase(it);
            return;
        }
    }

    auto* msg = ev->Get();
    auto now = GetCycleCount();

    ++VolumeRequestId;

    bool isTraced = false;

    if (ev->Recipient != ev->GetRecipientRewrite())
    {
        if (TraceSerializer->IsTraced(msg->CallContext->LWOrbit)) {
            isTraced = true;
            now = msg->Record.GetHeaders().GetInternal().GetTraceTs();
        } else if (TraceSerializer->HandleTraceRequest(
            msg->Record.GetHeaders().GetInternal().GetTrace(),
            msg->CallContext->LWOrbit))
        {
            isTraced = true;
            msg->Record.MutableHeaders()->MutableInternal()->SetTraceTs(now);
        }
    }

    LWTRACK(
        RequestReceived_Volume,
        msg->CallContext->LWOrbit,
        TMethod::Name,
        msg->CallContext->RequestId);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &ev->TraceId, this, msg);

    auto replyError = [&] (
        ui32 errorCode,
        TString errorMessage)
    {
        auto response = std::make_unique<typename TMethod::TResponse>(
            MakeError(errorCode, std::move(errorMessage)));

        FillResponse<TMethod>(*response, *msg->CallContext, now);

        NCloud::Reply(ctx, *ev, std::move(response));
    };

    if (ShuttingDown) {
        replyError(E_REJECTED, "Shutting down");
        return;
    }

    if (IsDiskRegistryMediaKind(State->GetConfig().GetStorageMediaKind())) {
        if (State->GetMeta().GetDevices().empty()) {
            replyError(E_REJECTED, TStringBuilder()
                << "Storage not allocated for volume: "
                << State->GetDiskId().Quote());
            return;
        }
    }

    if (State->GetPartitionsState() != TPartitionInfo::READY) {
        StartPartitionsIfNeeded(ctx);

        if (!State->Ready()) {
            if (RejectRequestIfNotReady<TMethod>()) {
                replyError(E_REJECTED, TStringBuilder()
                    << "Volume not ready: " << State->GetDiskId().Quote());
            } else {
                LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
                    "[%lu] %s request delayed until volume and partitions are ready",
                    TabletID(),
                    TMethod::Name);

                auto requestInfo = CreateRequestInfo<TMethod>(
                    ev->Sender,
                    ev->Cookie,
                    ev->Get()->CallContext,
                    ev->TraceId.Clone());

                PendingRequests.emplace_back(NActors::IEventHandlePtr(ev.Release()), requestInfo);
            }
            return;
        }
    }

    const auto& clientId = GetClientId(*msg);
    auto& clients = State->AccessClients();
    auto it = clients.end();

    bool throttlingDisabled = false;
    bool forceWrite = false;
    if (RequiresMount<TMethod>()) {
        it = clients.find(clientId);
        if (it == clients.end()) {
            replyError(E_BS_INVALID_SESSION, "Invalid session");
            return;
        }

        const auto& clientInfo = it->second;

        throttlingDisabled = HasProtoFlag(
            clientInfo.GetVolumeClientInfo().GetMountFlags(),
            NProto::MF_THROTTLING_DISABLED);

        if (RequiresThrottling<TMethod>() && throttlingDisabled) {
            VolumeSelfCounters->Cumulative.ThrottlerSkippedRequests.Increment(1);
        }

        forceWrite = HasProtoFlag(
            clientInfo.GetVolumeClientInfo().GetMountFlags(),
            NProto::MF_FORCE_WRITE);
    }

    if (RequiresReadWriteAccess<TMethod>()
            && State->GetRejectWrite()
            && !forceWrite)
    {
        replyError(E_REJECTED, "Writes blocked");
        return;
    }

    if (IsDiskRegistryMediaKind(State->GetConfig().GetStorageMediaKind())) {
        ChangeSessionId(*msg, clientId);
    }

    {
        auto error = Throttle<TMethod>(ctx, ev, throttlingDisabled);
        if (HasError(error)) {
            replyError(error.GetCode(), error.GetMessage());
            return;
        } else if (!ev) {
            // request postponed
            return;
        }
    }

    if (RequiresMount<TMethod>()) {
        Y_VERIFY(it != clients.end());

        auto& clientInfo = it->second;
        NProto::TError error;

        if (ev->Recipient != ev->GetRecipientRewrite()) {
            error = clientInfo.CheckPipeRequest(
                ev->Recipient,
                RequiresReadWriteAccess<TMethod>(),
                TMethod::Name,
                State->GetDiskId());
        } else {
            error = clientInfo.CheckLocalRequest(
                ev->Sender.NodeId(),
                RequiresReadWriteAccess<TMethod>(),
                TMethod::Name,
                State->GetDiskId());
        }

        if (FAILED(error.GetCode())) {
            replyError(error.GetCode(), error.GetMessage());
            return;
        }

        if (RequiresReadWriteAccess<TMethod>()) {
            if (CheckpointRequestQueue.size()
                    && CheckpointRequestQueue.front().ReqType == ECheckpointRequestType::Create
                    && State->GetPartitions().size() > 1)
            {
                replyError(E_REJECTED, TStringBuilder()
                    << "Request " << TMethod::Name
                    << " is not allowed during checkpoint creation");
                return;
            }
        }
    }

    // mediaKind condition is needed only because nonrepl requests are still
    // forwarded - TODO: check that this forwarding actually improves
    // performance - if not, get rid of custom nonrepl-related code here
    if (!IsDiskRegistryMediaKind(State->GetConfig().GetStorageMediaKind())
        && RequiresReadWriteAccess<TMethod>())
    {
        const auto range = BuildRequestBlockRange(
            *msg,
            State->GetBlockSize());
        if (!WriteAndZeroRequestsInFlight.TryAddRequest(VolumeRequestId, range)) {
            replyError(E_REJECTED, TStringBuilder()
                << "Request " << TMethod::Name
                << " intersects with inflight write or zero request"
                << " (block range: " << DescribeRange(range) << ")");
            return;
        }
    }

    if (State->GetNonreplicatedPartitionActor()) {
        SendRequestToNonreplicatedPartitions<TMethod>(
            ctx,
            ev,
            State->GetNonreplicatedPartitionActor(),
            isTraced
        );
    } else if (CanForwardToPartition<TMethod>(State->GetPartitions().size())) {
        SendRequestToPartition<TMethod>(ctx, ev, 0, now);
    } else if (!HandleRequest<TMethod>(ctx, ev, isTraced, now)) {
        WriteAndZeroRequestsInFlight.RemoveRequest(VolumeRequestId);

        replyError(
            E_REJECTED,
            TStringBuilder() << "Sglist destroyed"
        );
    }
}

#define BLOCKSTORE_FORWARD_REQUEST(name, ns)                                   \
    void TVolumeActor::Handle##name(                                           \
        const ns::TEv##name##Request::TPtr& ev,                                \
        const TActorContext& ctx)                                              \
    {                                                                          \
        BLOCKSTORE_VOLUME_COUNTER(name);                                       \
        ForwardRequest<ns::T##name##Method>(ctx, ev);                          \
    }                                                                          \
                                                                               \
    void TVolumeActor::Handle##name##Response(                                 \
        const ns::TEv##name##Response::TPtr& ev,                               \
        const NActors::TActorContext& ctx)                                     \
    {                                                                          \
        HandleResponse<ns::T##name##Method>(ctx, ev);                          \
    }                                                                          \
// BLOCKSTORE_FORWARD_REQUEST

BLOCKSTORE_FORWARD_REQUEST(ReadBlocks,               TEvService)
BLOCKSTORE_FORWARD_REQUEST(WriteBlocks,              TEvService)
BLOCKSTORE_FORWARD_REQUEST(ZeroBlocks,               TEvService)
BLOCKSTORE_FORWARD_REQUEST(CreateCheckpoint,         TEvService)
BLOCKSTORE_FORWARD_REQUEST(DeleteCheckpoint,         TEvService)
BLOCKSTORE_FORWARD_REQUEST(GetChangedBlocks,         TEvService)
BLOCKSTORE_FORWARD_REQUEST(ReadBlocksLocal,          TEvService)
BLOCKSTORE_FORWARD_REQUEST(WriteBlocksLocal,         TEvService)

BLOCKSTORE_FORWARD_REQUEST(DescribeBlocks,           TEvVolume)
BLOCKSTORE_FORWARD_REQUEST(GetUsedBlocks,            TEvVolume)
BLOCKSTORE_FORWARD_REQUEST(GetPartitionInfo,         TEvVolume)
BLOCKSTORE_FORWARD_REQUEST(CompactRange,             TEvVolume)
BLOCKSTORE_FORWARD_REQUEST(GetCompactionStatus,      TEvVolume)
BLOCKSTORE_FORWARD_REQUEST(DeleteCheckpointData,     TEvVolume)
BLOCKSTORE_FORWARD_REQUEST(RebuildMetadata,          TEvVolume)
BLOCKSTORE_FORWARD_REQUEST(GetRebuildMetadataStatus, TEvVolume)


#undef BLOCKSTORE_FORWARD_REQUEST

}   // namespace NCloud::NBlockStore::NStorage
