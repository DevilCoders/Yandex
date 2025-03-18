#include "time_stats_counters.h"

#include <util/string/cast.h>
#include <util/system/compiler.h>

namespace NTimeStats {
    TTimeStatsCounters& TTimeStatsCounters::AddBucket(const TDuration& timeBorder) {
        auto timeBorderNum = Formatter.Duration2Num(timeBorder);
        if (Buckets.empty() || timeBorderNum > Formatter.Duration2Num(Buckets.back().UpperBound)) {
            Buckets.push_back({timeBorder,
                               Root->GetCounter(ToString(timeBorderNum) + Formatter.AccuracyTitle, true)});
        } else {
            throw yexception() << "timeBorder should be greater than last timeBorder added! "
                                  " Adding "
                               << timeBorderNum << Formatter.AccuracyTitle << " after " << Formatter.Duration2Num(Buckets.back().UpperBound) << Formatter.AccuracyTitle << ".";
        }
        return *this;
    }

    void TTimeStatsCounters::ReportEvent(const TDuration& eventLifetime) {
        if (Y_UNLIKELY(Buckets.empty())) {
            throw yexception() << "No time buckets added.";
        }
        for (const auto& bucket : Buckets) {
            if (eventLifetime <= bucket.UpperBound) {
                bucket.Counter->Inc();
                return;
            }
        }
        Buckets.back().Counter->Inc();
    }

}
