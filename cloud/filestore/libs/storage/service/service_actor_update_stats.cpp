#include "service_actor.h"

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

void TStorageServiceActor::HandleUpdateStats(
    const TEvServicePrivate::TEvUpdateStats::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    Y_UNUSED(ev);

    StatsRegistry->UpdateStats(true);
    ScheduleUpdateStats(ctx);
}

} // namespace NCloud::NFileStore::NStorage