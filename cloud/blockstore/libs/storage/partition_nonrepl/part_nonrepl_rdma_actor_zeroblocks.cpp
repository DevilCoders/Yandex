#include "part_nonrepl_rdma_actor.h"

#include <cloud/blockstore/libs/rdma/protobuf.h>
#include <cloud/blockstore/libs/service_local/rdma_protocol.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/core/block_handler.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <util/generic/string.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

using TResponse = TEvService::TEvZeroBlocksResponse;

////////////////////////////////////////////////////////////////////////////////

struct TRdmaRequestContext: NRdma::IClientHandler
{
    TActorSystem* ActorSystem;
    TRequestInfoPtr RequestInfo;
    TAtomic Responses;
    NProto::TError Error;
    const ui32 RequestBlockCount;
    NActors::TActorId ParentActorId;
    ui64 RequestId;

    TRdmaRequestContext(
            TActorSystem* actorSystem,
            TRequestInfoPtr requestInfo,
            TAtomicBase requests,
            ui32 requestBlockCount,
            NActors::TActorId parentActorId,
            ui64 requestId)
        : ActorSystem(actorSystem)
        , RequestInfo(std::move(requestInfo))
        , Responses(requests)
        , RequestBlockCount(requestBlockCount)
        , ParentActorId(parentActorId)
        , RequestId(requestId)
    {}

    void HandleResult(NRdma::TClientRequest& req, size_t responseBytes)
    {
        auto* serializer = TBlockStoreProtocol::Serializer();
        auto buffer = req.ResponseBuffer.Head(responseBytes);
        auto [result, err] = serializer->Parse(buffer);

        if (HasError(err)) {
            Error = std::move(err);
        }
    }

    void HandleResponse(
        NRdma::TClientRequestPtr req,
        ui32 status,
        size_t responseBytes) override
    {
        if (FAILED(status)) {
            Error = MakeError(status, "Rdma error");
        } else {
            HandleResult(*req, responseBytes);
        }

        auto* dr = static_cast<TRdmaContext*>(req->Context);
        dr->Endpoint->FreeRequest(std::move(req));

        delete dr;

        if (AtomicDecrement(Responses) == 0) {
            auto response = std::make_unique<TResponse>(std::move(Error));
            TAutoPtr<IEventHandle> event(
                new IEventHandle(
                    RequestInfo->Sender,
                    {},
                    response.get()));
            response.release();
            ActorSystem->Send(event);

            auto completion =
                std::make_unique<TEvNonreplPartitionPrivate::TEvZeroBlocksCompleted>();
            auto& counters = *completion->Stats.MutableUserWriteCounters();
            completion->TotalCycles = RequestInfo->GetTotalCycles();

            counters.SetBlocksCount(RequestBlockCount);
            TAutoPtr<IEventHandle> completionEvent(
                new IEventHandle(
                    ParentActorId,
                    {},
                    completion.get(),
                    0,
                    RequestId));

            completion.release();
            ActorSystem->Send(completionEvent);

            delete this;
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionRdmaActor::HandleZeroBlocks(
    const TEvService::TEvZeroBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo<TEvService::TZeroBlocksMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        RequestReceived_Partition,
        requestInfo->CallContext->LWOrbit,
        "ZeroBlocks",
        requestInfo->CallContext->RequestId);

    auto blockRange = TBlockRange64::WithLength(
        msg->Record.GetStartIndex(),
        msg->Record.GetBlocksCount());

    TVector<TDeviceRequest> deviceRequests;
    bool ok = InitRequests<TEvService::TZeroBlocksMethod>(
        ctx,
        *requestInfo,
        blockRange,
        &deviceRequests
    );

    if (!ok) {
        return;
    }

    auto requestContext = std::make_unique<TRdmaRequestContext>(
        ctx.ActorSystem(),
        requestInfo,
        deviceRequests.size(),
        msg->Record.GetBlocksCount(),
        SelfId(),
        ++RequestId);

    auto* serializer = TBlockStoreProtocol::Serializer();

    struct TDeviceRequestInfo
    {
        NRdma::IClientEndpointPtr Endpoint;
        NRdma::TClientRequestPtr ClientRequest;
        std::unique_ptr<TRdmaContext> DeviceRequestContext;
    };

    TVector<TDeviceRequestInfo> requests;

    for (auto& r: deviceRequests) {
        auto& ep = AgentId2Endpoint[r.Device.GetAgentId()];
        Y_VERIFY(ep);
        auto dr = std::make_unique<TRdmaContext>();
        dr->Endpoint = ep;
        dr->RequestHandler = &*requestContext;

        NProto::TZeroDeviceBlocksRequest deviceRequest;
        deviceRequest.MutableHeaders()->CopyFrom(msg->Record.GetHeaders());
        deviceRequest.SetDeviceUUID(r.Device.GetDeviceUUID());
        deviceRequest.SetStartIndex(r.DeviceBlockRange.Start);
        deviceRequest.SetBlocksCount(r.DeviceBlockRange.Size());
        deviceRequest.SetBlockSize(PartConfig->GetBlockSize());
        deviceRequest.SetSessionId(msg->Record.GetSessionId());

        auto [req, err] = ep->AllocateRequest(
            &*dr,
            serializer->MessageByteSize(deviceRequest, 0),
            4_KB);

        if (HasError(err)) {
            LOG_ERROR(ctx, TBlockStoreComponents::PARTITION,
                "Failed to allocate rdma memory for ZeroDeviceBlocksRequest"
                ", error: %s",
                FormatError(err).c_str());

            for (auto& request: requests) {
                request.Endpoint->FreeRequest(std::move(request.ClientRequest));
            }

            NCloud::Reply(
                ctx,
                *requestInfo,
                std::make_unique<TResponse>(std::move(err)));

            return;
        }

        serializer->Serialize(
            req->RequestBuffer,
            TBlockStoreProtocol::ZeroDeviceBlocksRequest,
            deviceRequest,
            TContIOVector(nullptr, 0));

        requests.push_back({ep, std::move(req), std::move(dr)});
    }

    for (auto& request: requests) {
        request.Endpoint->SendRequest(
            std::move(request.ClientRequest),
            requestInfo->CallContext);
        request.DeviceRequestContext.release();
    }

    RequestsInProgress.insert(RequestId);
    requestContext.release();
}

}   // namespace NCloud::NBlockStore::NStorage
