#include "timedelta_checker.h"

#include "config_global.h"

#include <util/system/sanitizers.h>

#include <cmath>

namespace NAntiRobot {
    TTimeDeltaChecker::TTimeDeltaChecker()
        : LastTime(0)
        , Critical(false)
    {
        // This is neeeded to unpoison padding bytes after Critical.
        NSan::Unpoison(this, sizeof(*this));
    }

    void TTimeDeltaChecker::ProcessRequest(const TInstant& arrivalTime, bool isAccountable) {
        if (!isAccountable)
            return;

        const ui64 ms = arrivalTime.MicroSeconds();

        if (LastTime == 0) {
            LastTime = ms;
        } else {
            float delta = 0.0f;
            if (ms > LastTime) {
                delta = (float)(ms - LastTime);
                LastTime = ms;
            }

            Deltas.Push(delta);
        }

        Critical = CalcCritical();
    }

    bool TTimeDeltaChecker::CalcCritical() const {
        size_t numDeltas = Deltas.GetSize();

        size_t minDeltas = Min<size_t>(ANTIROBOT_DAEMON_CONFIG.TimeDeltaMinDeltas, MaxDeltas);

        if (numDeltas < minDeltas)
            return false;

        size_t numDeltasToCheck = (size_t)Max<float>(minDeltas, Min<float>(MaxDeltas, 20.0f * 1e6f / Deltas.GetLast()));
        if (numDeltas < numDeltasToCheck)
            return false;

        float minDelta = 1e20f;
        float maxDelta = 0.0f;

        for (size_t i = numDeltas - numDeltasToCheck; i < numDeltas; ++i) {
            float delta = Deltas.GetItem(i);
            minDelta = Min(minDelta, delta);
            maxDelta = Max(maxDelta, delta);
        }

        return maxDelta >= 1e-3f && fabs(minDelta / maxDelta - 1.0f) < ANTIROBOT_DAEMON_CONFIG.TimeDeltaMaxDeviation;
    }

}
