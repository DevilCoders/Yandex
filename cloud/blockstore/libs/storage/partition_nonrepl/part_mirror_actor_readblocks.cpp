#include "part_mirror_actor.h"

#include <cloud/blockstore/libs/storage/api/undelivered.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TMirrorPartitionActor::ReadBlocks(
    const typename TMethod::TRequest::TPtr& ev,
    const TActorContext& ctx)
{
    if (HasError(Status)) {
        Reply(
            ctx,
            *ev,
            std::make_unique<typename TMethod::TResponse>(Status));

        return;
    }

    const auto blockRange = TBlockRange64::WithLength(
        ev->Get()->Record.GetStartIndex(),
        ev->Get()->Record.GetBlocksCount());

    TActorId replicaActorId;
    const auto error = State.NextReadReplica(blockRange, &replicaActorId);
    if (HasError(error)) {
        Reply(
            ctx,
            *ev,
            std::make_unique<typename TMethod::TResponse>(error));

        return;
    }

    ForwardRequestWithNondeliveryTracking(ctx, replicaActorId, *ev);
}

////////////////////////////////////////////////////////////////////////////////

void TMirrorPartitionActor::HandleReadBlocks(
    const TEvService::TEvReadBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    ReadBlocks<TEvService::TReadBlocksMethod>(ev, ctx);
}

void TMirrorPartitionActor::HandleReadBlocksLocal(
    const TEvService::TEvReadBlocksLocalRequest::TPtr& ev,
    const TActorContext& ctx)
{
    ReadBlocks<TEvService::TReadBlocksLocalMethod>(ev, ctx);
}

}   // namespace NCloud::NBlockStore::NStorage
