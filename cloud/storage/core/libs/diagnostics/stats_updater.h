#pragma once

#include "public.h"

#include <cloud/storage/core/libs/common/startable.h>

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

struct IStats
{
    virtual ~IStats() = default;

    virtual void AddIncompleteRequest(const TIncompleteRequest& request) = 0;

    virtual void UpdateStats(bool updatePercentiles) = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct IStatsUpdater
    : public IStartable
{
    virtual ~IStatsUpdater() = default;
};

////////////////////////////////////////////////////////////////////////////////

IStatsUpdaterPtr CreateStatsUpdater(
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    IStatsPtr stats,
    TVector<IIncompleteRequestProviderPtr> incompleteRequestProviders);

}   // namespace NCloud
