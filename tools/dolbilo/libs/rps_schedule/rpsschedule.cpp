#include "rpsschedule.h"

#include <util/generic/algorithm.h>
#include <util/string/ascii.h>

template <>
void Out<TRpsSchedule>(IOutputStream& os, const TRpsSchedule& v) {
    bool first = true;
    for (const TRpsSchedule::value_type& el : v) {
        if (!first)
            os << " ";
        os << el;
        first = false;
    }
}

template <typename T>
int Signum(T x) {
    return (T(0) < x) - (x < T(0));
}

template <>
void Out<TRpsScheduleElement>(IOutputStream& os, const TRpsScheduleElement& el) {
    switch (el.Mode) {
        case SM_AT_UNIX:
            os << "at_unix(";
            break;
        case SM_STEP:
            os << "step(" << el.BeginRps << ", " << el.EndRps << ", " << el.StepRps << ", ";
            break;
        case SM_LINE:
            os << "line(" << el.BeginRps << ", " << el.EndRps << ", ";
            break;
        case SM_CONST:
            os << "const(" << el.BeginRps << ", ";
            break;
        default:
            Y_FAIL("unknown rps-schedule mode %d", el.Mode);
    }
    os << el.Duration << ")";
}

template <>
TRpsSchedule FromStringImpl<TRpsSchedule>(const char* data, size_t len) {
    TString stripped;
    RemoveCopyIf(data, data + len, std::back_inserter(stripped), [](char a) { return IsAsciiSpace(a); });
    TStringBuf s(stripped);
    TRpsSchedule ret;
    while (!s.empty()) {
        TRpsScheduleElement el;

        el.Mode = FromString<ESchedMode>(s.NextTok('('));
        switch (el.Mode) {
            case SM_AT_UNIX:
                if (ret)
                    ythrow yexception() << "at_unix() must be the first item in rps schedule";
                // TDuration is used instead of TInstant as there is no
                // TInstant("unix-timestamp") parser.
                el.Duration = TDuration::Parse(s.NextTok(')'));
                el.BeginRps = el.EndRps = el.StepRps = 0;
                break;
            case SM_STEP:
                el.BeginRps = FromString<float>(s.NextTok(','));
                el.EndRps = FromString<float>(s.NextTok(','));
                if (el.BeginRps == el.EndRps)
                    ythrow yexception() << "step() with same begin and end should be const()";
                el.StepRps = FromString<float>(s.NextTok(','));
                if (el.StepRps <= 0)
                    ythrow yexception() << "step() must have positive step";
                el.Duration = TDuration::Parse(s.NextTok(')'));
                break;
            case SM_LINE:
                el.BeginRps = FromString<float>(s.NextTok(','));
                el.EndRps = FromString<float>(s.NextTok(','));
                if (el.BeginRps == el.EndRps)
                    ythrow yexception() << "line() with same begin and end should be const()";
                el.Duration = TDuration::Parse(s.NextTok(')'));
                break;
            case SM_CONST:
                el.BeginRps = el.EndRps = FromString<float>(s.NextTok(','));
                el.Duration = TDuration::Parse(s.NextTok(')'));
                break;
            default:
                abort();
        }
        if (!(el.BeginRps >= 0 && el.EndRps >= 0 && el.StepRps >= 0))
            ythrow yexception() << "rps must be non-negative";
        if (el.Duration <= TDuration::Zero())
            ythrow yexception() << "duration must be positive";
        ret.push_back(el);
    }
    return ret;
}

TRpsScheduleIterator::TRpsScheduleIterator(const TRpsSchedule& sched, const TInstant& now)
    : Sched(sched)
    , Ndx(0)
    , StepEnds(MakeStepEnds(sched, now))
    , laststep(TDuration::Seconds(1))
{
    if (sched)
        Y_VERIFY(now != TInstant::Zero() || Sched[0].Mode == SM_AT_UNIX);
}

TVector<TInstant> TRpsScheduleIterator::MakeStepEnds(const TRpsSchedule& sched, const TInstant& now) {
    TVector<TInstant> ends(sched.size(), TInstant::Zero());

    TInstant last = now;
    for (size_t i = 0; i < sched.size(); ++i) {
        ends[i] = (sched[i].Mode == SM_AT_UNIX ? TInstant::Zero() : last) + sched[i].GetElementDuration();
        last = ends[i];
    }

    return ends;
}

typedef TVector<TInstant> TEnds;

template <>
void Out<TEnds>(IOutputStream& os, const TEnds& v) {
    bool first = true;
    os << "[";
    for (const TEnds::value_type& el : v) {
        if (!first)
            os << ", ";
        os << el;
        first = false;
    }
    os << "]";
}

TInstant TRpsScheduleIterator::NextShot(const TInstant& now) {
    for (; Ndx < StepEnds.size() && StepEnds[Ndx] <= now; ++Ndx) {
        ;
    }

    if (Ndx >= Sched.size())
        return TInstant::Zero();

    const TRpsScheduleElement& el = Sched[Ndx];
    const TInstant start = StepEnds[Ndx] - el.GetElementDuration();
    switch (el.Mode) {
        case SM_AT_UNIX:
            return StepEnds[Ndx];
        case SM_STEP: {
            const long int stepCount = el.CountLadderSteps();
            Y_VERIFY(start <= now);
            const long int stepNdx = lrint(floor(stepCount * ((now - start).SecondsFloat() / el.GetElementDuration().SecondsFloat())));
            Rps = el.BeginRps + stepNdx * el.StepRps * Signum(el.EndRps - el.BeginRps);
            if (Rps != 0)
                laststep = TDuration::Seconds(1) / Rps;
            else
                laststep = el.Duration;
            break;
        }
        case SM_LINE: {
            // Find `step > 0` so that `step * (RPS(now) + RPS(now+step)) / 2 == 1`.
            const double c = (el.EndRps - el.BeginRps);
            const double e = (now.SecondsFloat() - start.SecondsFloat());
            const double ec_ad = e * c + (el.BeginRps * el.GetElementDuration().SecondsFloat());
            const double underSqrt = ec_ad * ec_ad + 2. * c * el.GetElementDuration().SecondsFloat();
            bool good = false;
            if (underSqrt > 0) {
                const double step = (sqrt(underSqrt) - ec_ad) / c;
                if (step > 0) {
                    laststep = TDuration::Seconds(step);
                    good = true;
                }
            }
            if (!good) {
                Cdbg << "Can't process " << el << " at " << (now - start) << ", repeating laststep=" << laststep << "\n";
            }
            Rps = (c / el.GetElementDuration().SecondsFloat()) * e + el.BeginRps;
            break;
        }
        case SM_CONST:
            Rps = el.BeginRps;
            if (el.BeginRps) {
                laststep = TDuration::Seconds(1) / el.BeginRps;
            } else {
                return StepEnds[Ndx];
            }
            break;
        default:
            Y_FAIL("unknown rps-schedule mode %d", el.Mode);
    }

    if (laststep == TDuration::Zero()) {
        // That's safety net to avoid problems. Maybe it should rather ythrow.
        Cerr << "Can't process " << el << " at " << (now - start) << ", laststep=" << laststep << "\n";
        laststep = TDuration::Seconds(1);
    }

    if (StepEnds[Ndx] < now + laststep)
        return StepEnds[Ndx];

    return now + laststep;
}
