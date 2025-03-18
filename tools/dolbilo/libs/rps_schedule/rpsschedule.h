#pragma once

#include "schedmode.h"

#include <util/string/vector.h>
#include <util/datetime/base.h>
#include <util/generic/ymath.h>

struct TRpsScheduleElement {
    ESchedMode Mode;
    TDuration Duration; // that's start timestamp for SM_AT_UNIX, and single-step timestamp for step()
    double BeginRps;
    double EndRps;
    double StepRps;

    long int CountLadderSteps() const {
        Y_VERIFY(Mode == SM_STEP);
        return labs(lrint((EndRps - BeginRps) / StepRps)) + 1;
    }
    TDuration GetElementDuration() const {
        return (Mode == SM_STEP) ? (Duration * CountLadderSteps()) : Duration;
    }
};

typedef TVector<TRpsScheduleElement> TRpsSchedule;

class TRpsScheduleIterator {
public:
    TRpsScheduleIterator(const TRpsSchedule& sched, const TInstant& now = TInstant::Zero());
    TInstant NextShot(const TInstant& now);
    TInstant NextShot(TInstant now, unsigned int stepCount) {
        for (; stepCount > 0 && now != TInstant::Zero(); --stepCount) {
            now = NextShot(now);
        }
        return now;
    }
    TInstant GetFinish() const {
        Y_VERIFY(StepEnds.size() > 0);
        return StepEnds.back();
    }

    double GetRps() const {
        return Rps;
    }

private:
    static TVector<TInstant> MakeStepEnds(const TRpsSchedule& sched, const TInstant& now);

    const TRpsSchedule& Sched;
    size_t Ndx;
    const TVector<TInstant> StepEnds;
    TDuration laststep;

    // Shooting stats
    double Rps = 0.0;
};
