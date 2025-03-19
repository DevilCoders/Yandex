#include "part_nonrepl_migration_actor.h"

#include <cloud/blockstore/libs/storage/api/volume.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionMigrationActor::HandlePartCounters(
    const TEvVolume::TEvNonreplicatedPartitionCounters::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    if (ev->Sender == SrcActorId) {
        SrcCounters = std::move(msg->DiskCounters);
    } else if (ev->Sender == DstActorId) {
        DstCounters = std::move(msg->DiskCounters);
    } else {
        LOG_INFO(ctx, TBlockStoreComponents::PARTITION,
            "Partition %s for disk %s counters not found",
            ToString(ev->Sender).c_str(),
            SrcConfig->GetName().Quote().c_str());

        Y_VERIFY_DEBUG(0);
    }
}

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionMigrationActor::SendStats(const TActorContext& ctx)
{
    auto stats = CreatePartitionDiskCounters(EPublishingPolicy::NonRepl);

    if (SrcCounters) {
        stats->AggregateWith(*SrcCounters);
    }

    if (DstActorId && DstCounters) {
        stats->AggregateWith(*DstCounters);
    }

    auto request = std::make_unique<TEvVolume::TEvNonreplicatedPartitionCounters>(
        MakeIntrusive<TCallContext>(),
        std::move(stats));

    NCloud::Send(ctx, SrcConfig->GetParentActorId(), std::move(request));
}

}   // namespace NCloud::NBlockStore::NStorage
