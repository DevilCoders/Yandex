#pragma once

#include "time_stats.h"

namespace NAntiRobot {
    struct TRequestTimeStats {
        TTimeStats& AnswerTimeStats;
        TTimeStats& WaitTimeStats;
        TTimeStats* ReadTimeStats = nullptr;
        TTimeStats* CaptchaAnswerTimeStats = nullptr;

        TRequestTimeStats(TTimeStats& answerTimeStats, TTimeStats& waitTimeStats)
            : AnswerTimeStats(answerTimeStats)
            , WaitTimeStats(waitTimeStats)
        {
        }

        TRequestTimeStats(TTimeStats& answerTimeStats, TTimeStats& waitTimeStats, TTimeStats& readTimeStats)
            : AnswerTimeStats(answerTimeStats)
            , WaitTimeStats(waitTimeStats)
            , ReadTimeStats(&readTimeStats)
        {
        }

        TRequestTimeStats(TTimeStats& answerTimeStats, TTimeStats& waitTimeStats,
                          TTimeStats& readTimeStats, TTimeStats& captchaAnswerTimeStats)
            : AnswerTimeStats(answerTimeStats)
            , WaitTimeStats(waitTimeStats)
            , ReadTimeStats(&readTimeStats)
            , CaptchaAnswerTimeStats(&captchaAnswerTimeStats)
        {
        }
    };
}
