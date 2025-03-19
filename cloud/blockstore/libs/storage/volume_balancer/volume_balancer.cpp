#include "volume_balancer.h"

#include "volume_balancer_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

IActorPtr CreateVolumeBalancerActor(
    NKikimrConfig::TAppConfigPtr appConfig,
    TStorageConfigPtr storageConfig,
    IVolumeStatsPtr volumeStats,
    ICgroupStatsFetcherPtr cgroupStatFetcher,
    NActors::TActorId serviceActorId)
{
    return std::make_unique<TVolumeBalancerActor>(
        std::move(appConfig),
        std::move(storageConfig),
        std::move(volumeStats),
        std::move(cgroupStatFetcher),
        serviceActorId);
}

IActorPtr CreateVolumeBalancerActorStub()
{
    return std::make_unique<TVolumeBalancerActor>();
}

}   // namespace NCloud::NBlockStore::NStorage
