#pragma once

#include "config_global.h"

#include <library/cpp/timezone_conversion/convert.h>

#include <util/datetime/base.h>

namespace NAntiRobot {
    inline TDuration GetMskAndUtcTimeDiff() {
        static const auto now = TInstant::Now();
        static const NDatetime::TTimeZone msk = NDatetime::GetTimeZone("Europe/Moscow");
        static const NDatetime::TTimeZone utc = NDatetime::GetTimeZone("UTC");
        static const TDuration timeDiff = NDatetime::ToAbsoluteTime(NDatetime::ToCivilTime(now, msk), utc) - now;
        return timeDiff;
    }

    // Когда в Москве ночь, серваки отдыхают. И можно снизить пороги бана роботов.
    inline bool IsNightInMoscow(const TInstant& now) {
        TInstant nowInMoscow = now + GetMskAndUtcTimeDiff();
        ui8 hours = nowInMoscow.Hours() % 24;

        return ANTIROBOT_DAEMON_CONFIG.NightStartHour <= hours && hours < ANTIROBOT_DAEMON_CONFIG.NightEndHour;
    }
}
