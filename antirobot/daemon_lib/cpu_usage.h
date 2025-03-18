#pragma once

#include <antirobot/lib/stats_writer.h>

#include <util/datetime/base.h>

#include <atomic>


namespace NAntiRobot {

class TCpuUsage {
public:
    double Get() const {
        return CpuUsage;
    }

    void PrintStats(TStatsWriter& out) const;
    void Update();
private:
    TInstant PrevTimestamp{};
    TDuration PrevCpuTime{};

    std::atomic<double> CpuUsage{};
};

} // namespace NAntiRobot
