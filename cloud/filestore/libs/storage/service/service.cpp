#include "service.h"

#include "service_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

IActorPtr CreateStorageService(
    TStorageConfigPtr storageConfig,
    IRequestStatsRegistryPtr statsRegistry)
{
    return std::make_unique<TStorageServiceActor>(
        std::move(storageConfig),
        std::move(statsRegistry));
}

}   // namespace NCloud::NFileStore::NStorage
