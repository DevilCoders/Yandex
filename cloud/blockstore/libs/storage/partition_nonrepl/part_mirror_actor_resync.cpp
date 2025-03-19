#include "part_mirror_actor.h"

#include <cloud/blockstore/libs/storage/core/probes.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

// TODO: implement resync

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TMirrorPartitionActor::HandleRWClientIdChanged(
    const TEvVolume::TEvRWClientIdChanged::TPtr& ev,
    const TActorContext& ctx)
{
    auto rwClientId = ev->Get()->RWClientId;

    for (const auto& actorId: State.GetReplicaActors()) {
        NCloud::Send(
            ctx,
            actorId,
            std::make_unique<TEvVolume::TEvRWClientIdChanged>(rwClientId));
    }

    State.SetRWClientId(std::move(rwClientId));
}

}   // namespace NCloud::NBlockStore::NStorage
