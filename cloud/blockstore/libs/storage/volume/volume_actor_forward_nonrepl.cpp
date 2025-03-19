#include "volume_actor.h"

#include <cloud/blockstore/libs/storage/api/undelivered.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TVolumeActor::SendRequestToNonreplicatedPartitions(
    const TActorContext& ctx,
    const typename TMethod::TRequest::TPtr& ev,
    const TActorId& partActorId,
    bool isTraced)
{
    Y_UNUSED(isTraced); // TODO

    const bool processed = SendRequestToPartitionWithUsedBlockTracking<TMethod>(
        ctx,
        ev,
        partActorId);

    if (processed) {
        return;
    }

    ForwardRequestWithNondeliveryTracking(ctx, partActorId, *ev);
}

////////////////////////////////////////////////////////////////////////////////

#define GENERATE_IMPL(name, ns)                                                \
template void TVolumeActor::SendRequestToNonreplicatedPartitions<              \
    ns::T##name##Method>(                                                      \
        const TActorContext& ctx,                                              \
        const ns::TEv##name##Request::TPtr& ev,                                \
        const TActorId& partActorId,                                           \
        bool isTraced);                                                        \
// GENERATE_IMPL

GENERATE_IMPL(ReadBlocks,         TEvService)
GENERATE_IMPL(WriteBlocks,        TEvService)
GENERATE_IMPL(ZeroBlocks,         TEvService)
GENERATE_IMPL(CreateCheckpoint,   TEvService)
GENERATE_IMPL(DeleteCheckpoint,   TEvService)
GENERATE_IMPL(GetChangedBlocks,   TEvService)
GENERATE_IMPL(ReadBlocksLocal,    TEvService)
GENERATE_IMPL(WriteBlocksLocal,   TEvService)

GENERATE_IMPL(DescribeBlocks,           TEvVolume)
GENERATE_IMPL(GetUsedBlocks,            TEvVolume)
GENERATE_IMPL(GetPartitionInfo,         TEvVolume)
GENERATE_IMPL(CompactRange,             TEvVolume)
GENERATE_IMPL(GetCompactionStatus,      TEvVolume)
GENERATE_IMPL(DeleteCheckpointData,     TEvVolume)
GENERATE_IMPL(RebuildMetadata,          TEvVolume)
GENERATE_IMPL(GetRebuildMetadataStatus, TEvVolume)

#undef GENERATE_IMPL

}   // namespace NCloud::NBlockStore::NStorage
