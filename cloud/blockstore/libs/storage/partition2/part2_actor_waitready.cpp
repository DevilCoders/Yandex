#include "part2_actor.h"

namespace NCloud::NBlockStore::NStorage::NPartition2 {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleWaitReady(
    const TEvPartition::TEvWaitReadyRequest::TPtr& ev,
    const TActorContext& ctx)
{
    if (CurrentState != STATE_WORK) {
        LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
            "[%lu] WaitReady request delayed until partition is ready",
            TabletID());

        auto requestInfo = CreateRequestInfo<TEvPartition::TWaitReadyMethod>(
            ev->Sender,
            ev->Cookie,
            ev->Get()->CallContext,
            ev->TraceId.Clone());

        PendingRequests.emplace_back(NActors::IEventHandlePtr(ev.Release()), requestInfo);
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
        "[%lu] Received WaitReady request",
        TabletID());

    auto response = std::make_unique<TEvPartition::TEvWaitReadyResponse>();

    NCloud::Reply(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition2
