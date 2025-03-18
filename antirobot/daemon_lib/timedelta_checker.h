#pragma once

#include "cyclic_queue.h"
#include "timer.h"

#include <util/datetime/base.h>
#include <util/generic/typetraits.h>

namespace NAntiRobot {
    class TTimeDeltaChecker {
    public:
        TTimeDeltaChecker();

        void ProcessRequest(const TInstant& arrivalTime, bool isAccountable);

        bool IsCritical() const {
            return Critical;
        }

    private:
        enum {
            MaxDeltas = 30
        };

        TCyclicQueue<float, MaxDeltas> Deltas;
        ui64 LastTime;
        bool Critical;

        bool CalcCritical() const;
    };

    class TTimeDeltaExpDeviation {
        public:
            inline TTimeDeltaExpDeviation()
                : TimeDeltaMedian(0)
                , TimeDeltaDispersion(0)
                , NumTimeDeltas(0)
            {
            }

            inline float ProcessRequest(TInstant now) {
                RecalcTimeDeltaDistribParams(Timer.Next(now.MicroSeconds()));

                return GetDeviationFromExpDistrib();
            }

        private:
            inline void RecalcTimeDeltaDistribParams(float delta) {
                const float TimeDeltaMaxHistoryFactor = 0.99f;

                float historyFactor = Min(TimeDeltaMaxHistoryFactor, NumTimeDeltas / (NumTimeDeltas + 1.f));
                float deviation = delta - TimeDeltaMedian;
                TimeDeltaMedian = historyFactor * TimeDeltaMedian + (1 - historyFactor) * delta;
                TimeDeltaDispersion = historyFactor * TimeDeltaDispersion + (1 - historyFactor) * deviation * deviation;
                ++NumTimeDeltas;
            }

            inline float GetDeviationFromExpDistrib() const {
                return (TimeDeltaMedian * TimeDeltaMedian + 1) / (TimeDeltaDispersion + 1);
            }

        private:
            TTimer<ui64> Timer;
            float TimeDeltaMedian;
            float TimeDeltaDispersion;
            ui32 NumTimeDeltas;
    };

    class TTimeDelta {
    public:
        TTimeDelta()
            : LastDelta(0.0f)
        {
        }

        float GetLastDelta() const {
            return LastDelta;
        }

        void Next(ui64 nowMicroseconds) {
            LastDelta = float(Timer.Next(nowMicroseconds));
        }
    private:
        TTimer<ui64> Timer;
        float LastDelta;
    };
}

Y_DECLARE_PODTYPE(NAntiRobot::TTimeDeltaChecker);
Y_DECLARE_PODTYPE(NAntiRobot::TTimeDeltaExpDeviation);
Y_DECLARE_PODTYPE(NAntiRobot::TTimeDelta);
