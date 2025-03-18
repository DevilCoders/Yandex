#pragma once

#include <antirobot/lib/stats_writer.h>

#include <atomic>


namespace NAntiRobot {
    struct TVerochkaStats {
        std::atomic<size_t> InvalidVerochkaRequests = 0;

        void PrintStats(TStatsWriter& out) const {
            out.WriteScalar("invalid_verochka_requests", InvalidVerochkaRequests);
        }
    };
} // namespace NAntiRobot
