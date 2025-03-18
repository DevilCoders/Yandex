#pragma once

#include "time_accuracy.h"

#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NTimeStats {
    class TTimeStatsCounters {
    public:
        using TDynamicCountersPtr = TIntrusivePtr<NMonitoring::TDynamicCounters>;
        using TCounterPtr = TIntrusivePtr<NMonitoring::TCounterForPtr>;
        using TSelf = TTimeStatsCounters;

    public:
        TTimeStatsCounters(TDynamicCountersPtr root, ETimeAccuracy accuracy = ETimeAccuracy::MILLISECONDS)
            : Root(root)
            , Formatter(accuracy)
        {
        }

        /// All AddBucket calls should have ascending timeBorders (regarding the accuracy).
        /// @throws yexception if timeBorder is lessOrEqual to last timeBorder added.
        TSelf& AddBucket(const TDuration& timeBorder);

        /// container should be iterable and contain TDuration objects
        /// TDuration objects should be in ascending order (regarding the accuracy).
        template <typename C>
        TSelf& AddBuckets(const C& container) {
            for (const TDuration& timeBorder : container) {
                AddBucket(timeBorder);
            }
            return *this;
        }

        /// Thread-safe methods of reporting events' lifetimes
        void ReportEvent(const TDuration& eventLifetime);
        void ReportEvent(const TInstant& eventSpawnTime) {
            ReportEvent(Now() - eventSpawnTime);
        }

    private:
        struct TTimeBucket {
            TDuration UpperBound;
            TCounterPtr Counter;
        };

    private:
        TDynamicCountersPtr Root;
        TTimeAccuracyFormatter Formatter;
        TVector<TTimeBucket> Buckets;
    };

}

struct TTimeBucket {
    TDuration UpperBound;
    TString Title;
};
