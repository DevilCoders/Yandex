#include "rps.h"

#include <util/system/yassert.h>

namespace NStat {
    static inline size_t RoundUp(TDuration t, TDuration unit) {
        size_t ms = t.MicroSeconds();
        return ms ? (ms - 1) / unit.MicroSeconds() + 1 : 0;
    }

    static inline size_t RoundDown(TDuration t, TDuration unit) {
        return t.MicroSeconds() / unit.MicroSeconds();
    }

    TRpsCounter::TRpsCounter(TDuration windowSize, TDuration unitSize)
        : StartTime(Now())
        , UnitSize(unitSize)
        , Requests(RoundUp(windowSize, unitSize))
    {
    }

    inline size_t TRpsCounter::UnitIndex(TInstant now) const {
        return RoundDown(now - StartTime, UnitSize);
    }

    void TRpsCounter::RegisterRequest(TInstant now) {
        size_t index = UnitIndex(now);

        while (index >= Requests.TotalSize())
            Requests.PushBack(0);

        // register only if corresponding unit window is still available
        if (Y_LIKELY(Requests.IsAvail(index)))
            Requests[index] += 1;
    }

    float TRpsCounter::AverageRps(TInstant beg, TInstant end) const {
        size_t b = std::max(UnitIndex(beg), Requests.FirstIndex());
        size_t e = UnitIndex(std::min(end, Now()));

        if (e <= b)
            return 0;

        TDuration period = UnitSize * (e - b);
        e = std::min(e, Requests.TotalSize());

        size_t reqs = 0;
        for (; b < e; ++b)
            reqs += Requests[b];

        return reqs / period.SecondsFloat();
    }

}
