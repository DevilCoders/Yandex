#include "tablet.h"

#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

IActorPtr CreateIndexTablet(
    const TActorId& owner,
    TTabletStorageInfoPtr storage,
    TStorageConfigPtr config,
    IProfileLogPtr profileLog,
    IStorageCountersPtr storageCounters)
{
    return std::make_unique<TIndexTabletActor>(
        owner,
        std::move(storage),
        std::move(config),
        std::move(profileLog),
        std::move(storageCounters));
}

}   // namespace NCloud::NFileStore::NStorage
