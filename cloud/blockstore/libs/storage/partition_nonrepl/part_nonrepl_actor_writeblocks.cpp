#include "part_nonrepl_actor.h"
#include "part_nonrepl_util.h"

#include <cloud/blockstore/libs/common/iovector.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/core/block_handler.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/string.h>
#include <util/string/builder.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

class TDiskAgentWriteActor final
    : public TActorBootstrapped<TDiskAgentWriteActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    NProto::TWriteBlocksRequest Request;
    const TVector<TDeviceRequest> DeviceRequests;
    const TNonreplicatedPartitionConfigPtr PartConfig;
    const TActorId Part;

    ui32 RequestsCompleted = 0;

    bool ReplyLocal;

public:
    TDiskAgentWriteActor(
        TRequestInfoPtr requestInfo,
        NProto::TWriteBlocksRequest request,
        TVector<TDeviceRequest> deviceRequests,
        TNonreplicatedPartitionConfigPtr partConfig,
        const TActorId& part,
        bool replyLocal);

    void Bootstrap(const TActorContext& ctx);

private:
    void WriteBlocks(const TActorContext& ctx);

    bool HandleError(const TActorContext& ctx, NProto::TError error);

    void Done(const TActorContext& ctx, IEventBasePtr response, bool failed);

    IEventBasePtr CreateResponse(NProto::TError error);

private:
    STFUNC(StateWork);

    void HandleWriteDeviceBlocksResponse(
        const TEvDiskAgent::TEvWriteDeviceBlocksResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleWriteDeviceBlocksUndelivery(
        const TEvDiskAgent::TEvWriteDeviceBlocksRequest::TPtr& ev,
        const TActorContext& ctx);

    void HandleTimeout(
        const TEvents::TEvWakeup::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TDiskAgentWriteActor::TDiskAgentWriteActor(
        TRequestInfoPtr requestInfo,
        NProto::TWriteBlocksRequest request,
        TVector<TDeviceRequest> deviceRequests,
        TNonreplicatedPartitionConfigPtr partConfig,
        const TActorId& part,
        bool replyLocal)
    : RequestInfo(std::move(requestInfo))
    , Request(std::move(request))
    , DeviceRequests(std::move(deviceRequests))
    , PartConfig(std::move(partConfig))
    , Part(part)
    , ReplyLocal(replyLocal)
{
    ActivityType = TBlockStoreActivities::PARTITION_WORKER;
}

void TDiskAgentWriteActor::Bootstrap(const TActorContext& ctx)
{
    TRequestScope timer(*RequestInfo);

    Become(&TThis::StateWork);

    LWTRACK(
        RequestReceived_VolumeWorker,
        RequestInfo->CallContext->LWOrbit,
        "DiskAgentWrite",
        RequestInfo->CallContext->RequestId);

    WriteBlocks(ctx);
}

void TDiskAgentWriteActor::WriteBlocks(const TActorContext& ctx)
{
    TDeviceRequestBuilder builder(
        DeviceRequests,
        PartConfig->GetBlockSize(),
        Request);

    ui32 cookie = 0;
    for (const auto& deviceRequest: DeviceRequests) {
        auto request =
            std::make_unique<TEvDiskAgent::TEvWriteDeviceBlocksRequest>();
        request->Record.MutableHeaders()->CopyFrom(Request.GetHeaders());
        request->Record.SetDeviceUUID(deviceRequest.Device.GetDeviceUUID());
        request->Record.SetStartIndex(deviceRequest.DeviceBlockRange.Start);
        request->Record.SetBlockSize(PartConfig->GetBlockSize());
        request->Record.SetSessionId(Request.GetSessionId());

        builder.BuildNextRequest(request->Record);

        auto traceId = RequestInfo->TraceId.Clone();
        BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

        TAutoPtr<IEventHandle> event(
            new IEventHandle(
                MakeDiskAgentServiceId(deviceRequest.Device.GetNodeId()),
                ctx.SelfID,
                request.get(),
                IEventHandle::FlagForwardOnNondelivery,
                cookie++,
                &ctx.SelfID,    // forwardOnNondelivery
                std::move(traceId)
            ));
        request.release();

        ctx.Send(event);
    }
}

bool TDiskAgentWriteActor::HandleError(
    const TActorContext& ctx,
    NProto::TError error)
{
    if (FAILED(error.GetCode())) {
        if (error.GetCode() == E_BS_INVALID_SESSION) {
            NCloud::Send(
                ctx,
                PartConfig->GetParentActorId(),
                std::make_unique<TEvVolume::TEvReacquireDisk>()
            );

            error.SetCode(E_REJECTED);
        }

        if (error.GetCode() == E_IO) {
            error = PartConfig->MakeIOError(std::move(error.GetMessage()));
        }

        Done(ctx, CreateResponse(std::move(error)), true);
        return true;
    }

    return false;
}

IEventBasePtr TDiskAgentWriteActor::CreateResponse(NProto::TError error)
{
    if (ReplyLocal) {
        return std::make_unique<TEvService::TEvWriteBlocksLocalResponse>(
            std::move(error));
    }

    return std::make_unique<TEvService::TEvWriteBlocksResponse>(
        std::move(error));
}

void TDiskAgentWriteActor::Done(
    const TActorContext& ctx,
    IEventBasePtr response,
    bool failed)
{
    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);

    LWTRACK(
        ResponseSent_VolumeWorker,
        RequestInfo->CallContext->LWOrbit,
        "WriteBlocks",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));

    auto completion =
        std::make_unique<TEvNonreplPartitionPrivate::TEvWriteBlocksCompleted>();
    auto& counters = *completion->Stats.MutableUserWriteCounters();
    completion->TotalCycles = RequestInfo->GetTotalCycles();
    ui32 blocks = 0;
    for (const auto& dr: DeviceRequests) {
        blocks += dr.BlockRange.Size();
        completion->DeviceIndices.push_back(dr.DeviceIdx);
    }
    counters.SetBlocksCount(blocks);
    completion->Failed = failed;

    NCloud::Send(
        ctx,
        Part,
        std::move(completion),
        0,  // cookie
        std::move(RequestInfo->TraceId));

    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TDiskAgentWriteActor::HandleWriteDeviceBlocksUndelivery(
    const TEvDiskAgent::TEvWriteDeviceBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto& uuid = DeviceRequests[ev->Cookie].Device.GetDeviceUUID();

    LOG_WARN_S(ctx, TBlockStoreComponents::PARTITION_WORKER,
        "WriteBlocks undelivered for " << uuid);

    // Ignore undelivered event. Wait for TEvWakeup.
}

void TDiskAgentWriteActor::HandleTimeout(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    const auto& uuid = DeviceRequests[ev->Cookie].Device.GetDeviceUUID();

    LOG_WARN_S(ctx, TBlockStoreComponents::PARTITION_WORKER,
        "WriteBlocks request timed out. Disk id: " << PartConfig->GetName() <<
        " Device id: " << uuid);

    HandleError(ctx, MakeError(E_TIMEOUT, "WriteBlocks request timed out"));
}

void TDiskAgentWriteActor::HandleWriteDeviceBlocksResponse(
    const TEvDiskAgent::TEvWriteDeviceBlocksResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    if (HandleError(ctx, msg->GetError())) {
        return;
    }

    if (++RequestsCompleted < DeviceRequests.size()) {
        return;
    }

    Done(ctx, CreateResponse(NProto::TError()), false);
}

STFUNC(TDiskAgentWriteActor::StateWork)
{
    TRequestScope timer(*RequestInfo);

    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvWakeup, HandleTimeout);

        HFunc(
            TEvDiskAgent::TEvWriteDeviceBlocksRequest,
            HandleWriteDeviceBlocksUndelivery);
        HFunc(
            TEvDiskAgent::TEvWriteDeviceBlocksResponse,
            HandleWriteDeviceBlocksResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionActor::HandleWriteBlocks(
    const TEvService::TEvWriteBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo<TEvService::TWriteBlocksMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        RequestReceived_Partition,
        requestInfo->CallContext->LWOrbit,
        "WriteBlocks",
        requestInfo->CallContext->RequestId);

    auto replyError = [=] (
        const TActorContext& ctx,
        TRequestInfo& requestInfo,
        ui32 errorCode,
        TString errorReason)
    {
        auto response = std::make_unique<TEvService::TEvWriteBlocksLocalResponse>(
            MakeError(errorCode, std::move(errorReason)));

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo.TraceId, this, response);

        LWTRACK(
            ResponseSent_Partition,
            requestInfo.CallContext->LWOrbit,
            "WriteBlocks",
            requestInfo.CallContext->RequestId);

        NCloud::Reply(ctx, requestInfo, std::move(response));
    };

    for (const auto& buffer: msg->Record.GetBlocks().GetBuffers()) {
        if (buffer.Size() % PartConfig->GetBlockSize() != 0) {
            replyError(
                ctx,
                *requestInfo,
                E_ARGUMENT,
                TStringBuilder() << "buffer not divisible by blockSize: "
                    << buffer.Size() << " % " << PartConfig->GetBlockSize()
                    << " != 0");
            return;
        }
    }

    const auto blockRange = TBlockRange64::WithLength(
        msg->Record.GetStartIndex(),
        CalculateWriteRequestBlockCount(msg->Record, PartConfig->GetBlockSize())
    );

    TVector<TDeviceRequest> deviceRequests;
    TRequest request;
    bool ok = InitRequests<TEvService::TWriteBlocksMethod>(
        *msg,
        ctx,
        *requestInfo,
        blockRange,
        &deviceRequests,
        &request
    );

    if (!ok) {
        return;
    }

    auto actorId = NCloud::Register<TDiskAgentWriteActor>(
        ctx,
        requestInfo,
        std::move(msg->Record),
        std::move(deviceRequests),
        PartConfig,
        SelfId(),
        false); // replyLocal

    RequestsInProgress.emplace(actorId, std::move(request));
}

void TNonreplicatedPartitionActor::HandleWriteBlocksLocal(
    const TEvService::TEvWriteBlocksLocalRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo<TEvService::TWriteBlocksLocalMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        RequestReceived_Partition,
        requestInfo->CallContext->LWOrbit,
        "WriteBlocks",
        requestInfo->CallContext->RequestId);

    auto replyError = [=] (
        const TActorContext& ctx,
        TRequestInfo& requestInfo,
        ui32 errorCode,
        TString errorReason)
    {
        auto response = std::make_unique<TEvService::TEvWriteBlocksLocalResponse>(
            MakeError(errorCode, std::move(errorReason)));

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo.TraceId, this, response);

        LWTRACK(
            ResponseSent_Partition,
            requestInfo.CallContext->LWOrbit,
            "WriteBlocks",
            requestInfo.CallContext->RequestId);

        NCloud::Reply(ctx, requestInfo, std::move(response));
    };

    auto guard = msg->Record.Sglist.Acquire();

    if (!guard) {
        replyError(ctx, *requestInfo, E_CANCELLED, "Local request was cancelled");
        return;
    }

    auto blockRange = TBlockRange64::WithLength(
        msg->Record.GetStartIndex(),
        msg->Record.BlocksCount);

    TVector<TDeviceRequest> deviceRequests;
    TRequest request;
    bool ok = InitRequests<TEvService::TWriteBlocksLocalMethod>(
        *msg,
        ctx,
        *requestInfo,
        blockRange,
        &deviceRequests,
        &request
    );

    if (!ok) {
        return;
    }

    // convert local request to remote

    SgListCopy(
        guard.Get(),
        ResizeIOVector(
            *msg->Record.MutableBlocks(),
            msg->Record.BlocksCount,
            PartConfig->GetBlockSize()));

    auto actorId = NCloud::Register<TDiskAgentWriteActor>(
        ctx,
        requestInfo,
        std::move(msg->Record),
        std::move(deviceRequests),
        PartConfig,
        SelfId(),
        true); // replyLocal

    RequestsInProgress.emplace(actorId, std::move(request));
}

void TNonreplicatedPartitionActor::HandleWriteBlocksCompleted(
    const TEvNonreplPartitionPrivate::TEvWriteBlocksCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_TRACE(ctx, TBlockStoreComponents::PARTITION,
        "[%s] Complete write blocks", SelfId().ToString().c_str());

    const auto requestBytes = msg->Stats.GetUserWriteCounters().GetBlocksCount()
        * PartConfig->GetBlockSize();
    const auto time = CyclesToDurationSafe(msg->TotalCycles).MicroSeconds();
    PartCounters->RequestCounters.WriteBlocks.AddRequest(time, requestBytes);

    RequestsInProgress.erase(ev->Sender);
    if (!msg->Failed) {
        for (const auto i: msg->DeviceIndices) {
            DeviceStats[i] = {};
        }
    }

    if (RequestsInProgress.empty() && Poisoner) {
        ReplyAndDie(ctx);
    }
}

}   // namespace NCloud::NBlockStore::NStorage
