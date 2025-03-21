#include "part_nonrepl_rdma_actor.h"

#include <cloud/blockstore/libs/rdma/protobuf.h>
#include <cloud/blockstore/libs/service_local/rdma_protocol.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/core/block_handler.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/string.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

using TResponse = TEvService::TEvReadBlocksResponse;

////////////////////////////////////////////////////////////////////////////////

struct TRdmaRequestContext: NRdma::IClientHandler
{
    TActorSystem* ActorSystem;
    TRequestInfoPtr RequestInfo;
    TAtomic Responses;
    NProto::TReadBlocksResponse Response;
    NActors::TActorId ParentActorId;
    ui64 RequestId;

    TRdmaRequestContext(
            TActorSystem* actorSystem,
            TRequestInfoPtr requestInfo,
            TAtomicBase requests,
            ui32 blockCount,
            ui32 blockSize,
            NActors::TActorId parentActorId,
            ui64 requestId)
        : ActorSystem(actorSystem)
        , RequestInfo(std::move(requestInfo))
        , Responses(requests)
        , ParentActorId(parentActorId)
        , RequestId(requestId)
    {
        auto& buffers = *Response.MutableBlocks()->MutableBuffers();
        buffers.Reserve(blockCount);
        for (ui32 i = 0; i < blockCount; ++i) {
            buffers.Add()->resize(blockSize, 0);
        }
    }

    void HandleResult(
        TDeviceReadRequestContext& dr,
        NRdma::TClientRequest& req,
        size_t responseBytes)
    {
        auto* serializer = TBlockStoreProtocol::Serializer();
        auto buffer = req.ResponseBuffer.Head(responseBytes);
        auto [result, err] = serializer->Parse(buffer);

        if (HasError(err)) {
            *Response.MutableError() = std::move(err);
            return;
        }

        auto& blocks = *Response.MutableBlocks()->MutableBuffers();

        ui64 offset = 0;
        ui64 b = 0;
        while (offset < result.Data.Size()) {
            ui64 targetBlock = dr.StartIndexOffset + b;
            Y_VERIFY(targetBlock < static_cast<ui64>(blocks.size()));
            ui64 bytes = Min(
                result.Data.Size() - offset,
                blocks[targetBlock].Size());
            Y_VERIFY(bytes);

            memcpy(
                const_cast<char*>(blocks[targetBlock].Data()),
                result.Data.data() + offset,
                bytes);
            offset += bytes;
            ++b;
        }
    }

    void HandleResponse(
        NRdma::TClientRequestPtr req,
        ui32 status,
        size_t responseBytes) override
    {
        auto* dr = static_cast<TDeviceReadRequestContext*>(req->Context);

        if (FAILED(status)) {
            *Response.MutableError() = MakeError(status, "Rdma error");
        } else {
            HandleResult(*dr, *req, responseBytes);
        }

        dr->Endpoint->FreeRequest(std::move(req));

        delete dr;

        if (AtomicDecrement(Responses) == 0) {
            ui32 blocks = Response.GetBlocks().BuffersSize();

            auto response = std::make_unique<TResponse>();
            response->Record = std::move(Response);
            TAutoPtr<IEventHandle> event(
                new IEventHandle(
                    RequestInfo->Sender,
                    {},
                    response.get(),
                    0,
                    RequestInfo->Cookie));

            response.release();
            ActorSystem->Send(event);

            auto completion =
                std::make_unique<TEvNonreplPartitionPrivate::TEvReadBlocksCompleted>();
            auto& counters = *completion->Stats.MutableUserReadCounters();
            completion->TotalCycles = RequestInfo->GetTotalCycles();

            counters.SetBlocksCount(blocks);
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

void TNonreplicatedPartitionRdmaActor::HandleReadBlocks(
    const TEvService::TEvReadBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo<TEvService::TReadBlocksMethod>(
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

    auto blockRange = TBlockRange64::WithLength(
        msg->Record.GetStartIndex(),
        msg->Record.GetBlocksCount());

    TVector<TDeviceRequest> deviceRequests;
    bool ok = InitRequests<TEvService::TReadBlocksMethod>(
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
        blockRange.Size(),
        PartConfig->GetBlockSize(),
        SelfId(),
        ++RequestId);

    auto error = SendReadRequests(
        ctx,
        requestInfo->CallContext,
        msg->Record.GetHeaders(),
        msg->Record.GetSessionId(),
        &*requestContext,
        deviceRequests);

    if (HasError(error)) {
        NCloud::Reply(
            ctx,
            *requestInfo,
            std::make_unique<TResponse>(std::move(error)));

        return;
    }

    RequestsInProgress.insert(RequestId);
    requestContext.release();
}

}   // namespace NCloud::NBlockStore::NStorage
