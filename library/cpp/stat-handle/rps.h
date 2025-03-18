#pragma once

#include <library/cpp/containers/ring_buffer/ring_buffer.h>

#include <util/datetime/base.h>

namespace NStat {
    class TRpsCounter {
    public:
        // @windowSize specifies period of time for which rps data is available.
        // @unitSize is minimal precision of measuring, granularity of time. Default is 1 minute.
        TRpsCounter(TDuration windowSize,
                    TDuration unitSize = TDuration::Minutes(1));

        void RegisterRequest(TInstant now);

        void RegisterRequest() {
            RegisterRequest(Now());
        }

        // Average number of requests per second for specified period [@beg, @end].
        // This period is rounded up to minimal available whole units.
        float AverageRps(TInstant beg, TInstant end) const;

        // Average RPS for recent @period
        float RecentRps(TDuration period) const {
            TInstant now = Now();
            return AverageRps(now - period, now);
        }

    private:
        inline size_t UnitIndex(TInstant now) const;

    private:
        TInstant StartTime;
        TDuration UnitSize;

        // number of requests per unit of time
        TSimpleRingBuffer<size_t> Requests;
    };

}
