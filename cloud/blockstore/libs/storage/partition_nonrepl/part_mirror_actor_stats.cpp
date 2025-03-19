#include "part_mirror_actor.h"

#include <cloud/blockstore/libs/storage/api/volume.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

void TMirrorPartitionActor::HandlePartCounters(
    const TEvVolume::TEvNonreplicatedPartitionCounters::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    ui32 i = FindIndex(State.GetReplicaActors(), ev->Sender);

    if (i < ReplicaCounters.size()) {
        ReplicaCounters[i] = std::move(msg->DiskCounters);
    } else {
        LOG_INFO(ctx, TBlockStoreComponents::PARTITION,
            "Partition %s for disk %s counters not found",
            ToString(ev->Sender).c_str(),
            State.GetReplicaInfos()[0].Config->GetName().Quote().c_str());

        Y_VERIFY_DEBUG(0);
    }
}

////////////////////////////////////////////////////////////////////////////////

void TMirrorPartitionActor::SendStats(const TActorContext& ctx)
{
    auto stats = CreatePartitionDiskCounters(EPublishingPolicy::NonRepl);

    for (const auto& counters: ReplicaCounters) {
        if (counters) {
            stats->AggregateWith(*counters);
        }
    }

    auto request = std::make_unique<TEvVolume::TEvNonreplicatedPartitionCounters>(
        MakeIntrusive<TCallContext>(),
        std::move(stats));

    NCloud::Send(
        ctx,
        State.GetReplicaInfos()[0].Config->GetParentActorId(),
        std::move(request));
}

}   // namespace NCloud::NBlockStore::NStorage
