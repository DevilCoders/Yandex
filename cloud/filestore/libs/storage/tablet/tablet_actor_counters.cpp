#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;
using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::RegisterCounters(const TActorContext& ctx)
{
    if (!Counters) {
        auto counters = CreateIndexTabletCounters();

        // LAME: ownership transferred to executor
        Counters = counters.get();
        Executor()->RegisterExternalTabletCounters(counters.release());

        // only aggregated statistics will be reported by default
        // (you can always turn on per-tablet statistics on monitoring page)
        // TabletCountersAddTablet(TabletID(), ctx);

        ScheduleUpdateCounters(ctx);
    }
}

void TIndexTabletActor::RegisterStatCounters()
{
    if (!GetFileSystemId()) {
        // it's possible to have empty id for newly created volume
        // just wait for the config update
        return;
    }

    StatCounters = std::make_shared<TFileSystemStatCounters>(
        GetFileSystem(),
        StorageCounters);
}

void TIndexTabletActor::ScheduleUpdateCounters(const TActorContext& ctx)
{
    if (!UpdateCountersScheduled) {
        ctx.Schedule(UpdateCountersInterval,
            new TEvIndexTabletPrivate::TEvUpdateCounters());
        UpdateCountersScheduled = true;
    }
}

void TIndexTabletActor::HandleUpdateCounters(
    const TEvIndexTabletPrivate::TEvUpdateCounters::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    UpdateCounters();

    UpdateCountersScheduled = false;
    ScheduleUpdateCounters(ctx);
}

void TIndexTabletActor::UpdateCounters()
{
    if (StatCounters) {
        StatCounters->Update(GetFileSystemStats());
    }

#define FILESTORE_TABLET_UPDATE_COUNTER(name, ...)                             \
    {                                                                          \
        auto& counter = Counters->Simple()[                                    \
            TIndexTabletCounters::SIMPLE_COUNTER_Stats_##name];                \
        counter.Set(Get##name());                                              \
    }                                                                          \
// FILESTORE_TABLET_UPDATE_COUNTER

    FILESTORE_TABLET_STATS(FILESTORE_TABLET_UPDATE_COUNTER)

#undef FILESTORE_TABLET_UPDATE_COUNTER
}

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleGetStorageStats(
    const TEvIndexTablet::TEvGetStorageStatsRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto response = std::make_unique<TEvIndexTablet::TEvGetStorageStatsResponse>();

    auto* stats = response->Record.MutableStats();

#define FILESTORE_TABLET_UPDATE_COUNTER(name, ...)                             \
    stats->Set##name(Get##name());                                             \
// FILESTORE_TABLET_UPDATE_COUNTER

    FILESTORE_TABLET_STATS(FILESTORE_TABLET_UPDATE_COUNTER)

#undef FILESTORE_TABLET_UPDATE_COUNTER

    stats->SetTabletChannelCount(GetTabletChannelCount());
    stats->SetConfigChannelCount(GetConfigChannelCount());

    const auto txDeleteGarbageRwCompleted = Counters->TxCumulative(
        TIndexTabletCounters::ETransactionType::TX_DeleteGarbage,
        NKikimr::COUNTER_TT_RW_COMPLETED
    ).Get();
    stats->SetTxDeleteGarbageRwCompleted(txDeleteGarbageRwCompleted);

    response->Record.SetMediaKind(GetFileSystem().GetStorageMediaKind());

    NCloud::Reply(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
