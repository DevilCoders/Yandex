#include "part_nonrepl_rdma_actor.h"

#include <cloud/blockstore/libs/common/iovector.h>
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

using TResponse = TEvService::TEvReadBlocksLocalResponse;

////////////////////////////////////////////////////////////////////////////////

struct TRdmaRequestContext: NRdma::IClientHandler
{
    TActorSystem* ActorSystem;
    TRequestInfoPtr RequestInfo;
    TAtomic Responses;
    TGuardedSgList SgList;
    NProto::TError Error;
    const ui32 RequestBlockCount;
    NActors::TActorId ParentActorId;
    ui64 RequestId;

    TRdmaRequestContext(
            TActorSystem* actorSystem,
            TRequestInfoPtr requestInfo,
            TAtomicBase requests,
            TGuardedSgList sglist,
            ui32 requestBlockCount,
            NActors::TActorId parentActorId,
            ui64 requestId)
        : ActorSystem(actorSystem)
        , RequestInfo(std::move(requestInfo))
        , Responses(requests)
        , SgList(std::move(sglist))
        , RequestBlockCount(requestBlockCount)
        , ParentActorId(parentActorId)
        , RequestId(requestId)
    {
    }

    void HandleResult(
        TDeviceReadRequestContext& dr,
        NRdma::TClientRequest& req,
        size_t responseBytes)
    {
        if (auto guard = SgList.Acquire()) {
            auto* serializer = TBlockStoreProtocol::Serializer();
            auto buffer = req.ResponseBuffer.Head(responseBytes);
            auto [result, err] = serializer->Parse(buffer);

            if (HasError(err)) {
                Error = std::move(err);
                return;
            }

            TSgList data = guard.Get();
            ui64 offset = 0;
            ui64 b = 0;
            while (offset < result.Data.Size()) {
                ui64 targetBlock = dr.StartIndexOffset + b;
                Y_VERIFY(targetBlock < data.size());
                ui64 bytes = Min(
                    result.Data.Size() - offset,
                    data[targetBlock].Size());
                Y_VERIFY(bytes);

                memcpy(
                    const_cast<char*>(data[targetBlock].Data()),
                    result.Data.data() + offset,
                    bytes);
                offset += bytes;
                ++b;
            }
        }
    }

    void HandleResponse(
        NRdma::TClientRequestPtr req,
        ui32 status,
        size_t responseBytes) override
    {
        auto* dr = static_cast<TDeviceReadRequestContext*>(req->Context);

        if (FAILED(status)) {
            Error = MakeError(status, "Rdma error");
        } else {
            HandleResult(*dr, *req, responseBytes);
        }

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
                std::make_unique<TEvNonreplPartitionPrivate::TEvReadBlocksCompleted>();
            auto& counters = *completion->Stats.MutableUserReadCounters();
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

void TNonreplicatedPartitionRdmaActor::HandleReadBlocksLocal(
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
    bool ok = InitRequests<TEvService::TReadBlocksLocalMethod>(
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
        std::move(msg->Record.Sglist),
        msg->Record.GetBlocksCount(),
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
