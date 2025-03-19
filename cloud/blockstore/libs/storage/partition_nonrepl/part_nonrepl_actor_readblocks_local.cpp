#include "part_nonrepl_actor.h"

#include <cloud/blockstore/libs/common/iovector.h>
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

class TDiskAgentReadActor final
    : public TActorBootstrapped<TDiskAgentReadActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const NProto::TReadBlocksLocalRequest Request;
    const TVector<TDeviceRequest> DeviceRequests;
    const TNonreplicatedPartitionConfigPtr PartConfig;
    const TActorId Part;

    ui32 RequestsCompleted = 0;

public:
    TDiskAgentReadActor(
        TRequestInfoPtr requestInfo,
        NProto::TReadBlocksLocalRequest request,
        TVector<TDeviceRequest> deviceRequests,
        TNonreplicatedPartitionConfigPtr partConfig,
        const TActorId& part);

    void Bootstrap(const TActorContext& ctx);

private:
    void ReadBlocks(const TActorContext& ctx);

    bool HandleError(const TActorContext& ctx, NProto::TError error);

    void Done(const TActorContext& ctx, IEventBasePtr response, bool failed);

private:
    STFUNC(StateWork);

    void HandleReadDeviceBlocksResponse(
        const TEvDiskAgent::TEvReadDeviceBlocksResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleReadDeviceBlocksUndelivery(
        const TEvDiskAgent::TEvReadDeviceBlocksRequest::TPtr& ev,
        const TActorContext& ctx);

    void HandleTimeout(
        const TEvents::TEvWakeup::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TDiskAgentReadActor::TDiskAgentReadActor(
        TRequestInfoPtr requestInfo,
        NProto::TReadBlocksLocalRequest request,
        TVector<TDeviceRequest> deviceRequests,
        TNonreplicatedPartitionConfigPtr partConfig,
        const TActorId& part)
    : RequestInfo(std::move(requestInfo))
    , Request(std::move(request))
    , DeviceRequests(std::move(deviceRequests))
    , PartConfig(std::move(partConfig))
    , Part(part)
{
    ActivityType = TBlockStoreActivities::PARTITION_WORKER;
}

void TDiskAgentReadActor::Bootstrap(const TActorContext& ctx)
{
    TRequestScope timer(*RequestInfo);

    Become(&TThis::StateWork);

    LWTRACK(
        RequestReceived_VolumeWorker,
        RequestInfo->CallContext->LWOrbit,
        "DiskAgentRead",
        RequestInfo->CallContext->RequestId);

    ReadBlocks(ctx);
}

void TDiskAgentReadActor::ReadBlocks(const TActorContext& ctx)
{
    const auto blockSize = PartConfig->GetBlockSize();

    ui32 cookie = 0;
    for (const auto& deviceRequest: DeviceRequests) {
        auto request =
            std::make_unique<TEvDiskAgent::TEvReadDeviceBlocksRequest>();
        request->Record.MutableHeaders()->CopyFrom(Request.GetHeaders());
        request->Record.SetDeviceUUID(deviceRequest.Device.GetDeviceUUID());
        request->Record.SetStartIndex(deviceRequest.DeviceBlockRange.Start);
        request->Record.SetBlockSize(blockSize);
        request->Record.SetBlocksCount(deviceRequest.DeviceBlockRange.Size());
        request->Record.SetSessionId(Request.GetSessionId());

        auto traceId = RequestInfo->TraceId.Clone();
        BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

        TAutoPtr<IEventHandle> event(
            new IEventHandle(
                MakeDiskAgentServiceId(deviceRequest.Device.GetNodeId()),
                ctx.SelfID,
                request.get(),
                IEventHandle::FlagForwardOnNondelivery,
                cookie++,
                &ctx.SelfID, // forwardOnNondelivery
                std::move(traceId)
            ));
        request.release();

        ctx.Send(event);
    }
}

bool TDiskAgentReadActor::HandleError(
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

        auto response = std::make_unique<TEvService::TEvReadBlocksLocalResponse>(
            std::move(error)
        );

        Done(ctx, std::move(response), true);
        return true;
    }

    return false;
}

void TDiskAgentReadActor::Done(
    const TActorContext& ctx,
    IEventBasePtr response,
    bool failed)
{
    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);

    LWTRACK(
        ResponseSent_VolumeWorker,
        RequestInfo->CallContext->LWOrbit,
        "ReadBlocks",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));

    auto completion =
        std::make_unique<TEvNonreplPartitionPrivate::TEvReadBlocksCompleted>();
    auto& counters = *completion->Stats.MutableUserReadCounters();
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

void TDiskAgentReadActor::HandleReadDeviceBlocksUndelivery(
    const TEvDiskAgent::TEvReadDeviceBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto& uuid = DeviceRequests[ev->Cookie].Device.GetDeviceUUID();

    LOG_DEBUG_S(ctx, TBlockStoreComponents::PARTITION_WORKER,
        "ReadBlocks undelivered for " << uuid);

    // Ignore undelivered event. Wait for TEvWakeup.
}

void TDiskAgentReadActor::HandleTimeout(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    const auto& uuid = DeviceRequests[ev->Cookie].Device.GetDeviceUUID();

    LOG_WARN_S(ctx, TBlockStoreComponents::PARTITION_WORKER,
        "ReadBlocks request timed out. Disk id: " << PartConfig->GetName() <<
        " Device id: " << uuid);

    HandleError(ctx, MakeError(E_TIMEOUT, "ReadBlocks request timed out"));
}

void TDiskAgentReadActor::HandleReadDeviceBlocksResponse(
    const TEvDiskAgent::TEvReadDeviceBlocksResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    if (HandleError(ctx, msg->GetError())) {
        return;
    }

    auto guard = Request.Sglist.Acquire();

    if (!guard) {
        HandleError(ctx, MakeError(E_CANCELLED, "Local request was cancelled"));
        return;
    }

    const auto blockRange = DeviceRequests[ev->Cookie].BlockRange;

    if (blockRange.Size() != 0) {
        CopyToSgList(
            msg->Record.GetBlocks(),
            guard.Get(),
            blockRange.Start - Request.GetStartIndex(),
            PartConfig->GetBlockSize());
    }

    if (++RequestsCompleted < DeviceRequests.size()) {
        return;
    }

    auto response = std::make_unique<TEvService::TEvReadBlocksLocalResponse>();
    Done(ctx, std::move(response), false);
}

STFUNC(TDiskAgentReadActor::StateWork)
{
    TRequestScope timer(*RequestInfo);

    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvWakeup, HandleTimeout);

        HFunc(TEvDiskAgent::TEvReadDeviceBlocksRequest, HandleReadDeviceBlocksUndelivery);
        HFunc(TEvDiskAgent::TEvReadDeviceBlocksResponse, HandleReadDeviceBlocksResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionActor::HandleReadBlocksLocal(
    const TEvService::TEvReadBlocksLocalRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo<TEvService::TReadBlocksLocalMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        RequestReceived_Partition,
        requestInfo->CallContext->LWOrbit,
        "ReadBlocks",
        requestInfo->CallContext->RequestId);

    const auto blockRange = TBlockRange64::WithLength(
        msg->Record.GetStartIndex(),
        msg->Record.GetBlocksCount());

    TVector<TDeviceRequest> deviceRequests;
    TRequest request;
    bool ok = InitRequests<TEvService::TReadBlocksLocalMethod>(
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

    auto actorId = NCloud::Register<TDiskAgentReadActor>(
        ctx,
        requestInfo,
        std::move(msg->Record),
        std::move(deviceRequests),
        PartConfig,
        SelfId());

    RequestsInProgress.emplace(actorId, std::move(request));
}

}   // namespace NCloud::NBlockStore::NStorage
