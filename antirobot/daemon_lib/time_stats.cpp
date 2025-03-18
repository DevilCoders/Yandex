#include "time_stats.h"

#include <util/generic/strbuf.h>
#include <util/system/yassert.h>
#include <util/generic/algorithm.h>

namespace NAntiRobot {
    TTimeStats::TTimeStats(const TTimeStatInfoVector& timeStatInfos, const TString& statPrefix)
        : TimeStatInfos(&timeStatInfos)
        , StatPrefix(statPrefix)
    {
        Y_VERIFY(TimeStatInfos->size() <= MAX_STATS, "too many time stats");
        Y_VERIFY(IsSorted(TimeStatInfos->begin(), TimeStatInfos->end()), "time stats are not sorted");
        for (size_t i = 0; i < TimeStatInfos->size(); ++i) {
            TimeCountsLower[i] = 0;
            TimeCountsUpper[i] = 0;
        }
    }

    void TTimeStats::AddTime(TDuration duration) {
        for (size_t i = 0; i < TimeStatInfos->size(); ++i) {
            if (duration < (*TimeStatInfos)[i].Duration) {
                AtomicIncrement(TimeCountsLower[i]);
            } else {
                AtomicIncrement(TimeCountsUpper[i]);
            }
        }
    }

    void TTimeStats::AddTimeSince(TInstant time) {
        AddTime(TInstant::Now() - time);
    }

    void TTimeStats::PrintStats(TStatsWriter& out) const {
        for (size_t i = 0; i < TimeStatInfos->size(); ++i) {
            out.WriteScalar(StatPrefix + "time_" + (*TimeStatInfos)[i].Name, TimeCountsLower[i]);
            out.WriteScalar(StatPrefix + "time_upper_" + (*TimeStatInfos)[i].Name, TimeCountsUpper[i]);
        }
    }
}
