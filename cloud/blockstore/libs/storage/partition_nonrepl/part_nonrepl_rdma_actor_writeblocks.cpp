#include "part_nonrepl_rdma_actor.h"

#include "part_nonrepl_util.h"

#include <cloud/blockstore/libs/common/iovector.h>
#include <cloud/blockstore/libs/rdma/protobuf.h>
#include <cloud/blockstore/libs/service_local/rdma_protocol.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
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

struct TDeviceRequestInfo
{
    NRdma::IClientEndpointPtr Endpoint;
    NRdma::TClientRequestPtr ClientRequest;
    std::unique_ptr<TRdmaContext> DeviceRequestContext;
};

////////////////////////////////////////////////////////////////////////////////

struct TRdmaRequestContext: NRdma::IClientHandler
{
    TActorSystem* ActorSystem;
    TRequestInfoPtr RequestInfo;
    TAtomic Responses;
    bool ReplyLocal;
    NProto::TError Error;
    const ui32 RequestBlockCount;
    NActors::TActorId ParentActorId;
    ui64 RequestId;

    TRdmaRequestContext(
            TActorSystem* actorSystem,
            TRequestInfoPtr requestInfo,
            TAtomicBase requests,
            bool replyLocal,
            ui32 requestBlockCount,
            NActors::TActorId parentActorId,
            ui64 requestId)
        : ActorSystem(actorSystem)
        , RequestInfo(std::move(requestInfo))
        , Responses(requests)
        , ReplyLocal(replyLocal)
        , RequestBlockCount(requestBlockCount)
        , ParentActorId(parentActorId)
        , RequestId(requestId)
    {}

    IEventBasePtr MakeResponse()
    {
        if (ReplyLocal) {
            return std::make_unique<TEvService::TEvWriteBlocksLocalResponse>(
                std::move(Error));
        }

        return std::make_unique<TEvService::TEvWriteBlocksResponse>(
            std::move(Error));
    }

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
            auto response = MakeResponse();
            TAutoPtr<IEventHandle> event(
                new IEventHandle(
                    RequestInfo->Sender,
                    {},
                    response.get()));
            response.release();
            ActorSystem->Send(event);

            auto completion =
                std::make_unique<TEvNonreplPartitionPrivate::TEvWriteBlocksCompleted>();
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

void TNonreplicatedPartitionRdmaActor::HandleWriteBlocks(
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
    bool ok = InitRequests<TEvService::TWriteBlocksMethod>(
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
        false,
        blockRange.Size(),
        SelfId(),
        ++RequestId);

    auto* serializer = TBlockStoreProtocol::Serializer();

    TDeviceRequestBuilder builder(
        deviceRequests,
        PartConfig->GetBlockSize(),
        msg->Record);

    TVector<TDeviceRequestInfo> requests;

    for (auto& r: deviceRequests) {
        auto& ep = AgentId2Endpoint[r.Device.GetAgentId()];
        Y_VERIFY(ep);
        auto dr = std::make_unique<TRdmaContext>();
        dr->Endpoint = ep;
        dr->RequestHandler = &*requestContext;

        NProto::TWriteDeviceBlocksRequest deviceRequest;
        deviceRequest.MutableHeaders()->CopyFrom(msg->Record.GetHeaders());
        deviceRequest.SetDeviceUUID(r.Device.GetDeviceUUID());
        deviceRequest.SetStartIndex(r.DeviceBlockRange.Start);
        deviceRequest.SetBlockSize(PartConfig->GetBlockSize());
        deviceRequest.SetSessionId(msg->Record.GetSessionId());

        auto [req, err] = ep->AllocateRequest(
            &*dr,
            serializer->MessageByteSize(
                deviceRequest,
                r.DeviceBlockRange.Size() * PartConfig->GetBlockSize()),
            4_KB);

        if (HasError(err)) {
            LOG_ERROR(ctx, TBlockStoreComponents::PARTITION,
                "Failed to allocate rdma memory for WriteDeviceBlocksRequest, "
                " error: %s",
                FormatError(err).c_str());

            for (auto& request: requests) {
                request.Endpoint->FreeRequest(std::move(request.ClientRequest));
            }

            using TResponse = TEvService::TEvWriteBlocksResponse;
            NCloud::Reply(
                ctx,
                *requestInfo,
                std::make_unique<TResponse>(std::move(err)));

            return;
        }

        TVector<IOutputStream::TPart> parts;
        builder.BuildNextRequest(&parts);

        serializer->Serialize(
            req->RequestBuffer,
            TBlockStoreProtocol::WriteDeviceBlocksRequest,
            deviceRequest,
            TContIOVector(parts.data(), parts.size()));

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

void TNonreplicatedPartitionRdmaActor::HandleWriteBlocksLocal(
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
    bool ok = InitRequests<TEvService::TWriteBlocksLocalMethod>(
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
        true,
        blockRange.Size(),
        SelfId(),
        ++RequestId);

    auto* serializer = TBlockStoreProtocol::Serializer();
    const auto& sglist = guard.Get();

    TVector<TDeviceRequestInfo> requests;

    ui64 blocks = 0;
    for (auto& r: deviceRequests) {
        auto& ep = AgentId2Endpoint[r.Device.GetAgentId()];
        Y_VERIFY(ep);
        auto dr = std::make_unique<TRdmaContext>();
        dr->Endpoint = ep;
        dr->RequestHandler = &*requestContext;

        NProto::TWriteDeviceBlocksRequest deviceRequest;
        deviceRequest.MutableHeaders()->CopyFrom(msg->Record.GetHeaders());
        deviceRequest.SetDeviceUUID(r.Device.GetDeviceUUID());
        deviceRequest.SetStartIndex(r.DeviceBlockRange.Start);
        deviceRequest.SetBlockSize(PartConfig->GetBlockSize());
        deviceRequest.SetSessionId(msg->Record.GetSessionId());

        auto [req, err] = ep->AllocateRequest(
            &*dr,
            serializer->MessageByteSize(
                deviceRequest,
                r.DeviceBlockRange.Size() * PartConfig->GetBlockSize()),
            4_KB);

        if (HasError(err)) {
            LOG_ERROR(ctx, TBlockStoreComponents::PARTITION,
                "Failed to allocate rdma memory for WriteDeviceBlocksRequest"
                ", error: %s",
                FormatError(err).c_str());

            for (auto& request: requests) {
                request.Endpoint->FreeRequest(std::move(request.ClientRequest));
            }

            using TResponse = TEvService::TEvWriteBlocksLocalResponse;
            NCloud::Reply(
                ctx,
                *requestInfo,
                std::make_unique<TResponse>(std::move(err)));

            return;
        }

        serializer->Serialize(
            req->RequestBuffer,
            TBlockStoreProtocol::WriteDeviceBlocksRequest,
            deviceRequest,
            // XXX (cast)
            TContIOVector(
                const_cast<IOutputStream::TPart*>(
                    reinterpret_cast<const IOutputStream::TPart*>(
                        sglist.begin() + blocks
                )),
                r.DeviceBlockRange.Size()
            ));

        blocks += r.DeviceBlockRange.Size();

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
