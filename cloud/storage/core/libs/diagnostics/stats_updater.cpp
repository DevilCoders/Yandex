#include "stats_updater.h"

#include "incomplete_requests.h"

#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>

#include <library/cpp/deprecated/atomic/atomic.h>

namespace NCloud {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TStatsUpdater
    : public IStatsUpdater
    , public std::enable_shared_from_this<TStatsUpdater>
{
private:
    const ITimerPtr Timer;
    const ISchedulerPtr Scheduler;
    const IStatsPtr Stats;
    const TVector<IIncompleteRequestProviderPtr> IncompleteRequestProviders;

    TAtomic ShouldStop = 0;

    static constexpr size_t BUCKET_COUNT = 15;
    size_t UpdateStatsCounter = 0;

public:
    TStatsUpdater(
        ITimerPtr timer,
        ISchedulerPtr scheduler,
        IStatsPtr stats,
        TVector<IIncompleteRequestProviderPtr> incompleteRequestProviders);

    void Start() override;
    void Stop() override;

private:
    void UpdateStats();
    void ScheduleUpdateStats();
};

////////////////////////////////////////////////////////////////////////////////

TStatsUpdater::TStatsUpdater(
        ITimerPtr timer,
        ISchedulerPtr scheduler,
        IStatsPtr stats,
        TVector<IIncompleteRequestProviderPtr> incompleteRequestProviders)
    : Timer(std::move(timer))
    , Scheduler(std::move(scheduler))
    , Stats(std::move(stats))
    , IncompleteRequestProviders(std::move(incompleteRequestProviders))
{}

void TStatsUpdater::Start()
{
    ScheduleUpdateStats();
}

void TStatsUpdater::Stop()
{
    AtomicSet(ShouldStop, 1);
}

void TStatsUpdater::UpdateStats()
{
    bool updatePercentiles = (++UpdateStatsCounter % BUCKET_COUNT == 0);

    for (auto& incompleteRequestProvider : IncompleteRequestProviders) {
        auto incompleteRequests = incompleteRequestProvider->GetIncompleteRequests();
        for (const auto& request: incompleteRequests) {
            Stats->AddIncompleteRequest(request);
        }
    }

    Stats->UpdateStats(updatePercentiles);
}

void TStatsUpdater::ScheduleUpdateStats()
{
    if (AtomicGet(ShouldStop)) {
        return;
    }

    auto weak_ptr = weak_from_this();

    Scheduler->Schedule(
        Timer->Now() + UpdateStatsInterval,
        [=] {
            if (auto p = weak_ptr.lock()) {
                p->UpdateStats();
                p->ScheduleUpdateStats();
            }
        });
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IStatsUpdaterPtr CreateStatsUpdater(
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    IStatsPtr stats,
    TVector<IIncompleteRequestProviderPtr> incompleteRequestProviders)
{
    return std::make_unique<TStatsUpdater>(
        std::move(timer),
        std::move(scheduler),
        std::move(stats),
        std::move(incompleteRequestProviders));
}

}   // namespace NCloud
