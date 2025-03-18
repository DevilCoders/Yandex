#pragma once

#include <antirobot/lib/stats_writer.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NAntiRobot {
    struct TTimeStatInfo {
        TString Name;
        TDuration Duration;

        bool operator<(const TTimeStatInfo& rhs) const {
            return Duration < rhs.Duration;
        }
    };

    typedef TVector<TTimeStatInfo> TTimeStatInfoVector;

    const TTimeStatInfo TIME_STATS[] = {
        {"0.5ms", TDuration::MicroSeconds(500)},
        {"1ms",   TDuration::MilliSeconds(1)},
        {"5ms",   TDuration::MilliSeconds(5)},
        {"10ms",  TDuration::MilliSeconds(10)},
        {"30ms",  TDuration::MilliSeconds(30)},
        {"50ms",  TDuration::MilliSeconds(50)},
        {"90ms",  TDuration::MilliSeconds(90)},
        {"100ms", TDuration::MilliSeconds(100)},
        {"10s",   TDuration::Seconds(10)},
    };

    const TTimeStatInfo TIME_STATS_CACHER[] = {
        {"0.5ms", TDuration::MicroSeconds(500)},
        {"1ms",   TDuration::MilliSeconds(1)},
        {"5ms",   TDuration::MilliSeconds(5)},
        {"10ms",  TDuration::MilliSeconds(10)},
        {"30ms",  TDuration::MilliSeconds(30)},
        {"50ms",  TDuration::MilliSeconds(50)},
        {"90ms",  TDuration::MilliSeconds(90)},
        {"100ms", TDuration::MilliSeconds(100)},
        {"190ms", TDuration::MilliSeconds(190)},
        {"10s",   TDuration::Seconds(10)},
    };

    const TTimeStatInfo TIME_STATS_LARGE[] = {
        {"1ms",   TDuration::MilliSeconds(1)},
        {"5ms",   TDuration::MilliSeconds(5)},
        {"10ms",  TDuration::MilliSeconds(10)},
        {"30ms",  TDuration::MilliSeconds(30)},
        {"50ms",  TDuration::MilliSeconds(50)},
        {"100ms", TDuration::MilliSeconds(100)},
        {"200ms", TDuration::MilliSeconds(200)},
        {"300ms", TDuration::MilliSeconds(300)},
        {"400ms", TDuration::MilliSeconds(400)},
        {"500ms", TDuration::MilliSeconds(500)},
        {"700ms", TDuration::MilliSeconds(700)},
        {"10s",   TDuration::Seconds(10)},
    };

    const TTimeStatInfoVector TIME_STATS_VEC(TIME_STATS, TIME_STATS + Y_ARRAY_SIZE(TIME_STATS));
    const TTimeStatInfoVector TIME_STATS_VEC_CACHER(TIME_STATS_CACHER, TIME_STATS_CACHER + Y_ARRAY_SIZE(TIME_STATS_CACHER));
    const TTimeStatInfoVector TIME_STATS_VEC_LARGE(TIME_STATS_LARGE, TIME_STATS_LARGE + Y_ARRAY_SIZE(TIME_STATS_LARGE));

    class TTimeStats {
    public:
        TTimeStats() = default;
        TTimeStats(const TTimeStatInfoVector& timeStatInfos, const TString& statPrefix);
        void AddTime(TDuration duration);
        void AddTimeSince(TInstant time);
        void PrintStats(TStatsWriter& out) const;

    private:
        enum {MAX_STATS = 100};
        const TTimeStatInfoVector* TimeStatInfos;
        TAtomic TimeCountsLower[MAX_STATS];
        TAtomic TimeCountsUpper[MAX_STATS];
        TString StatPrefix;
    };

    class TMeasureDuration {
    public:
        explicit TMeasureDuration(TTimeStats& timeStats, TDuration elapsed = TDuration::Zero())
            : TimeStats(&timeStats)
            , Start(TInstant::Now() - elapsed)
        {
        }

        ~TMeasureDuration() {
            TimeStats->AddTime(TInstant::Now() - Start);
        }

        void ResetStats(TTimeStats& timeStats) {
            TimeStats = &timeStats;
        }

    private:
        TTimeStats* TimeStats;
        TInstant Start;
    };

    class TPausableMeasureDuration {
    public:
        explicit TPausableMeasureDuration(TTimeStats& timeStats, TDuration elapsed = TDuration::Zero())
            : TimeStats(timeStats)
            , Start(TInstant::Now())
            , Elapsed(elapsed)
        {
        }

        void Pause() {
            if (Paused) {
                return;
            }
            Paused = true;
            Elapsed += TInstant::Now() - Start;
        }

        void Resume() {
            if (!Paused) {
                return;
            }
            Paused = false;
            Start = TInstant::Now();
        }

        ~TPausableMeasureDuration() {
            if (!Paused) {
                Elapsed += TInstant::Now() - Start;
            }
            TimeStats.AddTime(Elapsed);
        }

    private:
        TTimeStats& TimeStats;
        TInstant Start;
        TDuration Elapsed;
        bool Paused = false;
    };
}
