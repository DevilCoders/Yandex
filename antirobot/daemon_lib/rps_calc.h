#pragma once

#include "timer.h"

#include <util/datetime/base.h>

namespace NAntiRobot {
    class TDiscreteRpsCounter {
    public:
        explicit TDiscreteRpsCounter(float smoothFactor = 0.5f)
            : LastArrival(TInstant::Zero())
            , CurrentSecondReqs(0)
            , SmoothFactor(smoothFactor)
            , SmoothRps(0.0)
        {
        }

        void Clear() {
            *this = TDiscreteRpsCounter(SmoothFactor);
        }

        void ProcessRequest(const TInstant& arrival) {
            if (arrival.Seconds() <= LastArrival.Seconds()) {
                ++CurrentSecondReqs;
            } else {
                auto diff = arrival.Seconds() - LastArrival.Seconds();
                float recentRps = static_cast<float>(CurrentSecondReqs) / diff;

                SmoothRps = recentRps * SmoothFactor + SmoothRps * (1.f - SmoothFactor);

                CurrentSecondReqs = 1;
                LastArrival = arrival;
            }
        }

        float GetRps() const {
            return SmoothRps;
        }

    private:
        TInstant LastArrival;
        size_t CurrentSecondReqs;
        float SmoothFactor;
        float SmoothRps;
    };
}
