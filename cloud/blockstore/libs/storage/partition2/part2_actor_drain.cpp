#include "part2_actor.h"

#include "part2_counters.h"

#include <cloud/blockstore/libs/storage/core/probes.h>

namespace NCloud::NBlockStore::NStorage::NPartition2 {

using namespace NActors;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleDrain(
    const TEvPartition::TEvDrainRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        RequestReceived_Partition,
        requestInfo->CallContext->LWOrbit,
        "Drain",
        requestInfo->CallContext->RequestId);

    const ui32 maxDrainRequests = 10;
    if (WriteAndZeroRequestsInProgress
            && DrainRequests.size() < maxDrainRequests)
    {
        DrainRequests.push_back(std::move(requestInfo));
        return;
    }

    std::unique_ptr<TEvPartition::TEvDrainResponse> response;
    if (WriteAndZeroRequestsInProgress) {
        response = std::make_unique<TEvPartition::TEvDrainResponse>(
            MakeError(E_REJECTED, "too many drain requests enqueued")
        );
    } else {
        response = std::make_unique<TEvPartition::TEvDrainResponse>();
    }

    BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

    LWTRACK(
        ResponseSent_Partition,
        requestInfo->CallContext->LWOrbit,
        "Drain",
        requestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *requestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::ProcessDrainRequests(const TActorContext& ctx)
{
    if (!WriteAndZeroRequestsInProgress) {
        for (auto& requestInfo: DrainRequests) {
            auto response = std::make_unique<TEvPartition::TEvDrainResponse>();

            BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

            LWTRACK(
                ResponseSent_Partition,
                requestInfo->CallContext->LWOrbit,
                "Drain",
                requestInfo->CallContext->RequestId);

            NCloud::Reply(ctx, *requestInfo, std::move(response));
        }

        DrainRequests.clear();
    }
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition2
