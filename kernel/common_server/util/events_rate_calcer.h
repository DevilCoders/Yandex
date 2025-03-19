#pragma once

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/datetime/base.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>

template <size_t EventsCapacity = 100 * 1024>
class TEventRate {
private:
    TAtomic Current;
    TVector<TInstant> Events;

private:
    inline size_t GetIndex(size_t current) const {
        return current % EventsCapacity;
    }

    inline TInstant GetEvent(size_t pos) const {
        return Events[GetIndex(pos)];
    }

public:
    TEventRate()
        : Current(0)
        , Events(EventsCapacity, TInstant::Zero())
    {}

    inline void Hit() {
        const size_t i = GetIndex(AtomicIncrement(Current));
        Y_ASSERT(i < EventsCapacity);
        Events[i] = Now();
    }

    void GetInterval(TDuration period, TInstant& begin, TInstant& end, ui64& count) const {
        const size_t step = 1;
        const size_t current = AtomicGet(Current);
        const TInstant border = Now() - period;

        end = GetEvent(current);
        begin = end;
        size_t i = current;
        for (; (current < EventsCapacity || i > current - EventsCapacity); i -= step) {
            TInstant t = GetEvent(i);
            if (t == TInstant::Zero() || t > begin)
                break;

            if (t < border)
                break;

            begin = t;
        }
        count = current - i;
    }

    float Get(const TDuration period) const {
        const TInstant start = Now();
        TInstant begin, end;
        ui64 count;
        GetInterval(period, begin, end, count);
        const TDuration passed = start - begin;
        const float secondsPassed = passed.SecondsFloat();
        return (fabs(secondsPassed)) > 1e-5 ? (float(count) / secondsPassed) : 0.f;
    }

public: // compatibility with TEventsRateCalcer
    void Rate(TMap<ui32, float>& rates) const {
        for (auto i = rates.begin(); i != rates.end(); ++i) {
            const TDuration period = TDuration::Seconds(i->first);
            i->second = Get(period);
        }
    }

    void AddRate(TMap<ui32, float>& rates) const {
        for (auto i = rates.begin(); i != rates.end(); ++i) {
            const TDuration period = TDuration::Seconds(i->first);
            i->second += Get(period);
        }
    }
};
