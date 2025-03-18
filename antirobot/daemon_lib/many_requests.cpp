#include "many_requests.h"


#include "req_types.h"

#include <library/cpp/deprecated/atomic/atomic.h>


namespace NAntiRobot {

void TManyRequestsStats::Update(const TRequest& req) {
    if (req.Host.starts_with("m.")) {
        Counters.Inc(req, ECounter::ManyRequestsMobile);
    } else {
        Counters.Inc(req, ECounter::ManyRequests);
    }
}

void TManyRequestsStats::PrintStats(TStatsWriter& out) const {
    Counters.Print(out);
}

} // namespace NAntiRobot
