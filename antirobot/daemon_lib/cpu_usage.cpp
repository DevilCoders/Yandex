#include "cpu_usage.h"

#include <sys/resource.h>

namespace NAntiRobot {

void TCpuUsage::PrintStats(TStatsWriter& out) const {
    out.WriteHistogram("cpu_usage", Get());
}

void TCpuUsage::Update() {
    const TInstant now = TInstant::Now();
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
        const TDuration cpuTime = TDuration(ru.ru_utime) + TDuration(ru.ru_stime);

        if (PrevTimestamp && now > PrevTimestamp) {
            CpuUsage = (cpuTime - PrevCpuTime) / (now - PrevTimestamp);
        }

        PrevTimestamp = now;
        PrevCpuTime = cpuTime;

    }
}

} // namespace NAntiRobot
