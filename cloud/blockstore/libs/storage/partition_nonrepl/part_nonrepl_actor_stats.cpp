#include "part_nonrepl_actor.h"

#include <cloud/blockstore/libs/storage/api/stats_service.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionActor::SendStats(const TActorContext& ctx)
{
    PartCounters->Simple.IORequestsInFlight.Set(RequestsInProgress.size());
    PartCounters->Simple.BytesCount.Set(
        PartConfig->GetBlockCount() * PartConfig->GetBlockSize()
    );

    auto request = std::make_unique<TEvVolume::TEvNonreplicatedPartitionCounters>(
        MakeIntrusive<TCallContext>(),
        std::move(PartCounters));

    PartCounters = CreatePartitionDiskCounters(EPublishingPolicy::NonRepl);

    NCloud::Send(ctx, StatActorId, std::move(request));
}

}   // namespace NCloud::NBlockStore::NStorage
