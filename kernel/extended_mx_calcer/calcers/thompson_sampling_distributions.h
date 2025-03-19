#pragma once

#include <util/generic/hash.h>
#include <util/random/fast.h>

namespace NExtendedMx {
    struct SurplusCounters {
        float win;
        float loss;
        float shows;

        SurplusCounters(float win, float loss, float shows) : win(win), loss(loss), shows(shows) {};
    };

    enum EDistribution {
        Beta /* "beta" */,
        WinLossBeta /* "win_loss_beta" */,
        WinLossShows /* "win_loss_shows" */,
    };

    class IBaseThompsonSamplingDistribution {
    public:
        virtual float Sample(TFastRng<ui32>& rng) const = 0;
        virtual ~IBaseThompsonSamplingDistribution() = default;
    };

    using TDistributionHolder = THolder<IBaseThompsonSamplingDistribution>;

    TDistributionHolder BuildDistribution(const SurplusCounters& counters, const EDistribution& distribution);
}
