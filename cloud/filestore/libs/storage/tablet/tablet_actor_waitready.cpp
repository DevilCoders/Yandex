#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleWaitReady(
    const TEvIndexTablet::TEvWaitReadyRequest::TPtr& ev,
    const TActorContext& ctx)
{
    if (CurrentState != STATE_WORK) {
        LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
            "[%lu] WaitReady request delayed until partition is ready",
            TabletID());

        WaitReadyRequests.emplace_back(ev.Release());
        return;
    }

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] Received WaitReady request",
        TabletID());

    auto response = std::make_unique<TEvIndexTablet::TEvWaitReadyResponse>();
    NCloud::Reply(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
