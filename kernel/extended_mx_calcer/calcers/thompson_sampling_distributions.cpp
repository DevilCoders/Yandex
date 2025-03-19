#include "thompson_sampling_distributions.h"

#include <quality/functionality/rtx/lib/beta/sampling.h>
#include <util/generic/serialized_enum.h>
#include <util/generic/utility.h>
#include <util/string/cast.h>
#include <util/string/vector.h>


namespace NExtendedMx {
    class TBetaDistribution : public IBaseThompsonSamplingDistribution {
    public:
        TBetaDistribution(float win, float loss, float /*shows*/) : Alpha(win + 1), Beta(loss + 1) {};

        float Sample(TFastRng<ui32>& rng) const override {
            return BetaRandom(Alpha, Beta, rng);
        }
    private:
        const float Alpha;
        const float Beta;
    };

    class TWinLossBetaDistribution : public IBaseThompsonSamplingDistribution {
    public:
        TWinLossBetaDistribution(float win, float loss, float shows) : WinAlpha(win + 1), LossAlpha(loss + 1), Beta(shows + 1) {};

        float Sample(TFastRng<ui32>& rng) const override {
            // Expectation of x ~ Beta(a, b) is equal to a / (a + b)
            // So expecation of win ~ Beta(wins, shows) is equal to E = wins / (wins + shows)
            // Thus mean_win = wins / shows = E / (1 - E)
            const float winSample = ExpInverseSigmoid(BetaRandom(WinAlpha, Beta, rng));
            const float lossSample = ExpInverseSigmoid(BetaRandom(LossAlpha, Beta, rng));
            return winSample - lossSample;
        }
    private:
        float ExpInverseSigmoid(float proba) const {
            if (1 - proba) {
                return proba / (1 - proba);
            } else {
                return std::numeric_limits<float>::infinity();
            }
        }
    private:
        const float WinAlpha;
        const float LossAlpha;
        const float Beta;
    };

    class TWinLossShowsDistribution : public IBaseThompsonSamplingDistribution {
    public:
        TWinLossShowsDistribution(float win, float loss, float shows) : WinAlpha(win + 1), LossAlpha(loss + 1), Shows(shows + 2) {};

        float Sample(TFastRng<ui32>& rng) const override {
            const float winSample = BetaRandom(WinAlpha, Max<float>(Shows - WinAlpha, 1.0), rng);
            const float lossSample = BetaRandom(LossAlpha, Max<float>(Shows - LossAlpha, 1.0), rng);
            return winSample - lossSample;
        }
    private:
        const float WinAlpha;
        const float LossAlpha;
        const float Shows;
    };

    TDistributionHolder BuildDistribution(const SurplusCounters& counters, const EDistribution& distribution) {
        switch (distribution) {
            case EDistribution::Beta:
                return MakeHolder<TBetaDistribution>(counters.win, counters.loss, counters.shows);
            case EDistribution::WinLossBeta:
                return MakeHolder<TWinLossBetaDistribution>(counters.win, counters.loss, counters.shows);
            case EDistribution::WinLossShows:
                return MakeHolder<TWinLossShowsDistribution>(counters.win, counters.loss, counters.shows);
            default:
                ythrow yexception() << "unknown distribution: " << distribution;
        }
    }
}
