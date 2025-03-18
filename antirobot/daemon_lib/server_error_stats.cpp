#include "server_error_stats.h"

#include "req_types.h"

#include <library/cpp/deprecated/atomic/atomic.h>


namespace NAntiRobot {

void TServerErrorStats::Update(const TRequest& req) {
    Histograms.Inc(req, EHistogram::ServerError);
}

void TServerErrorStats::PrintStats(TStatsWriter& out) const {
    Histograms.Print(out);
}

} // namespace NAntiRobot
