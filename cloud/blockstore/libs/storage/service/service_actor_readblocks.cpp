#include "service_actor.h"

#include <cloud/blockstore/libs/common/block_range.h>
#include <cloud/blockstore/libs/common/iovector.h>
#include <cloud/blockstore/libs/storage/api/undelivered.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/block_handler.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

class TReadBlocksRemoteRequestActor final
    : public TActorBootstrapped<TReadBlocksRemoteRequestActor>
{
private:
    const TEvService::TEvReadBlocksLocalRequest::TPtr Request;
    const ui64 BlockSize;
    const TActorId VolumeClient;

public:
    TReadBlocksRemoteRequestActor(
            TEvService::TEvReadBlocksLocalRequest::TPtr request,
            ui64 blockSize,
            TActorId volumeClient)
        : Request(request)
        , BlockSize(blockSize)
        , VolumeClient(volumeClient)
    {
        ActivityType = TBlockStoreActivities::SERVICE;
    }

    void Bootstrap(const TActorContext& ctx)
    {
        ConvertToRemoteAndSend(ctx);
        Become(&TThis::StateWait);
    }

private:
    void ConvertToRemoteAndSend(const TActorContext& ctx)
    {
        const auto* msg = Request->Get();
        const auto& clientId = GetClientId(*msg);
        const auto& diskId = GetDiskId(*msg);

        BLOCKSTORE_TRACE_RECEIVED(ctx, &Request->TraceId, this, Request->Get());

        auto request = CreateRemoteRequest();

        LOG_TRACE(ctx, TBlockStoreComponents::SERVICE,
            "Converted Local to gRPC ReadBlocks request for client %s to volume %s",
            clientId.Quote().data(),
            diskId.Quote().data());

        BLOCKSTORE_TRACE_SENT(ctx, &Request->TraceId, this, request);

        auto undeliveredActor = SelfId();

        auto event = std::make_unique<IEventHandle>(
            VolumeClient,
            SelfId(),
            request.release(),
            Request->Flags | IEventHandle::FlagForwardOnNondelivery,  // flags
            Request->Cookie,  // cookie
            &undeliveredActor,    // forwardOnNondelivery
            Request->TraceId.Clone());

        ctx.Send(event.release());
    }

    std::unique_ptr<TEvService::TEvReadBlocksRequest> CreateRemoteRequest()
    {
        const auto* msg = Request->Get();

        auto request = std::make_unique<TEvService::TEvReadBlocksRequest>(msg->CallContext);
        request->Record.MutableHeaders()->CopyFrom(msg->Record.GetHeaders());
        request->Record.SetDiskId(msg->Record.GetDiskId());
        request->Record.SetStartIndex(msg->Record.GetStartIndex());
        request->Record.SetBlocksCount(msg->Record.GetBlocksCount());
        request->Record.SetFlags(msg->Record.GetFlags());
        request->Record.SetCheckpointId(msg->Record.GetCheckpointId());
        request->Record.SetSessionId(msg->Record.GetSessionId());

        // TODO: CommitId
        return request;
    }

private:
    STFUNC(StateWait)
    {
        switch (ev->GetTypeRewrite()) {
            HFunc(TEvService::TEvReadBlocksResponse, HandleReadBlocksResponse);

            HFunc(TEvService::TEvReadBlocksRequest, HandleUndelivery);

            default:
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
                break;
        }
    }

    void HandleUndelivery(
        const TEvService::TEvReadBlocksRequest::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto* msg = ev->Get();

        BLOCKSTORE_TRACE_RECEIVED(ctx, &Request->TraceId, this, msg, &ev->TraceId);

        auto response = std::make_unique<TEvService::TEvReadBlocksLocalResponse>(
            MakeError(E_REJECTED, "Tablet is dead"));

        BLOCKSTORE_TRACE_SENT(ctx, &Request->TraceId, this, response);
        NCloud::Reply(ctx, *Request, std::move(response));

        Die(ctx);
    }

    void HandleReadBlocksResponse(
        const TEvService::TEvReadBlocksResponse::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto* msg = ev->Get();

        BLOCKSTORE_TRACE_RECEIVED(ctx, &Request->TraceId, this, msg, &ev->TraceId);

        auto error = msg->GetError();

        if (SUCCEEDED(error.GetCode())) {
            auto sgListOrError = GetSgList(msg->Record, BlockSize);

            if (HasError(sgListOrError)) {
                error = sgListOrError.GetError();
            } else if (auto guard = Request->Get()->Record.Sglist.Acquire()) {
                const auto& src = sgListOrError.GetResult();
                size_t bytesCopied = SgListCopy(src, guard.Get());
                Y_VERIFY(bytesCopied == Request->Get()->Record.GetBlocksCount() * BlockSize);
            } else {
                error.SetCode(E_REJECTED);
                error.SetMessage("failed to fill output buffer");
            }
        }

        LOG_TRACE(ctx, TBlockStoreComponents::SERVICE,
            "Converted ReadBlocks response using gRPC IPC to Local IPC");

        auto response = std::make_unique<TEvService::TEvReadBlocksLocalResponse>(
            error);

        BLOCKSTORE_TRACE_SENT(ctx, &Request->TraceId, this, response);
        NCloud::Reply(ctx, *Request, std::move(response));

        Die(ctx);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr CreateReadBlocksRemoteActor(
    TEvService::TEvReadBlocksLocalRequest::TPtr request,
    ui64 blockSize,
    TActorId volumeClient)
{
    return std::make_unique<TReadBlocksRemoteRequestActor>(
        std::move(request),
        blockSize,
        volumeClient);
}

}   // namespace NCloud::NBlockStore::NStorage
