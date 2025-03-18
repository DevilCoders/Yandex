#pragma once

#include "stat.h"
#include "time_stats.h"

#include <antirobot/lib/stats_writer.h>

namespace NAntiRobot {

class TServerTimeStat {
public:
    TServiceParamHolder<TTimeStats> Stats;

    explicit TServerTimeStat(const TTimeStatInfoVector& timeStatInfos, const TString& statPrefix) {
        for (auto& stat : Stats.GetArray()) {
            stat = TTimeStats(timeStatInfos, statPrefix);
        }
    }

    void PrintStats(TStatsWriter& out) const {
        for (size_t service = 0; service < HOST_NUMTYPES; ++service) {
            if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService && service != HOST_OTHER) {
                continue;
            }
            const auto serviceStr = ToString(static_cast<EHostType>(service));
            auto prefixedOut = out.WithTag("service_type", serviceStr);
            Stats.GetByService(service).PrintStats(prefixedOut);
        }
    }
};

}
