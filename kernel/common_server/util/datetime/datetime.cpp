#include "datetime.h"

#include <util/draft/datetime.h>
#include <kernel/common_server/util/math/math.h>
#include <util/datetime/base.h>
#include <util/string/cast.h>
#include <sstream>
#include <iomanip>

namespace {
    TInstant InstantOffset(const TInstant instant, const i64 offset) {
        return TInstant::Seconds(NMath::Add(instant.Seconds(), offset));
    }
}

TInstant TTimeZoneHelper::Gmt(const TInstant instant) const {
    return InstantOffset(instant, -Offset);
}

TInstant TTimeZoneHelper::Local(const TInstant instant) const {
    return InstantOffset(instant, Offset);
}

TString TTimeZoneHelper::FormatLocalDate(const TInstant instant) const {
    return Local(instant).FormatGmTime("%Y-%m-%d");
}

TString TTimeZoneHelper::FormatLocal(const TInstant instant, const char* format) const {
    return Local(instant).FormatGmTime(format);
}

i64 TTimeZoneHelper::Seconds() const {
    return Offset;
}

i64 TTimeZoneHelper::Minutes() const {
    return Offset / static_cast<i64>(TDuration::Minutes(1).Seconds());
}

i64 TTimeZoneHelper::Hours() const {
    return Offset / static_cast<i64>(TDuration::Hours(1).Seconds());
}

TTimeZoneHelper TTimeZoneHelper::FromSeconds(const i64 offset) {
    return TTimeZoneHelper(offset);
}

TTimeZoneHelper TTimeZoneHelper::FromMinutes(const i64 offset) {
    return TTimeZoneHelper(offset * static_cast<i64>(TDuration::Minutes(1).Seconds()));
}

TTimeZoneHelper TTimeZoneHelper::FromHours(const i64 offset) {
    return TTimeZoneHelper(offset * static_cast<i64>(TDuration::Hours(1).Seconds()));
}

TTimeZoneHelper TTimeZoneHelper::Default() {
    return TTimeZoneHelper::FromHours(3);
}

TTimeZoneHelper::TTimeZoneHelper(i64 offset)
    : Offset(offset) {
}

TLocalInstant::TLocalInstant(const TInstant& instant, const TTimeZoneHelper& timeZone)
    : Instant(instant)
    , TimeZone(timeZone) {
}

TInstant TLocalInstant::Gmt() const {
    return Instant;
}

TString TLocalInstant::FormatLocalTime() const {
    return TimeZone.FormatLocal(Instant, "%H:%M");
}

TInstant TLocalInstant::Local() const {
    return TimeZone.Local(Instant);
}

ui8 TLocalInstant::DayOfWeek() const {
    tm tm;
    Local().GmTime(&tm);
    // Starting from Monday (not Sunday)
    return (tm.tm_wday + 6) % 7;
}

bool TDateTimeParser::Parse(const TString& dateTime, const TString& dtTemplate, TInstant& result) {
    TStringBuf buf(dateTime.data(), dateTime.size());
    return Parse(buf, dtTemplate, result);
}

bool TDateTimeParser::Parse(TStringBuf dateTime, const TString& dtTemplate, TInstant& result) {
    std::tm timestamp = {};
    std::istringstream ss(dateTime.data());
    ss >> std::get_time(&timestamp, dtTemplate.data());
    if (ss.fail()) {
        return false;
    }
    result = TInstant::Seconds(std::mktime(&timestamp));
    return true;
}



tm GetTm(TInstant timestamp) {
    tm result;
    timestamp.GmTime(&result);
    return result;
}

namespace NUtil {

    TInstant ShiftCalendarMonths(const TInstant timestamp, const i32 diff) {
    #if (__cplusplus >= 202002)
        const auto timeOfDay = timestamp - TInstant::Days(timestamp.Days());
        const auto originalDate = std::chrono::year_month_day{std::chrono::sys_days{std::chrono::days{timestamp.Days()}}};
        auto finalDate = originalDate + std::chrono::months{diff};
        if (!finalDate.ok()) {
            finalDate = finalDate.year() / finalDate.month() / std::chrono::last;  // It's not a division operator, it's Gregorian calendar date creation.
        }
        return TInstant::Days(std::chrono::sys_days{finalDate}.time_since_epoch().count()) + timeOfDay;
    #else
        const TDuration timeOfDay = timestamp - TInstant::Days(timestamp.Days());
        const TInstant middayPoint = timestamp - timeOfDay + TDuration::Hours(12);

        auto tm = GetTm(middayPoint);
        tm.tm_mon += diff;
        while (tm.tm_mon < 0) {
            tm.tm_mon += 12;
            tm.tm_year -= 1;
        }
        while (tm.tm_mon >= 12) {
            tm.tm_mon -= 12;
            tm.tm_year += 1;
        }
        tm.tm_mday = std::min<int>(
            tm.tm_mday,
            NDatetime::MonthDays[NDatetime::LeapYearAD(tm.tm_year) ? 1 : 0][tm.tm_mon]
        );
        std::mktime(&tm);
        const auto time = TimeGM(&tm);

        const TInstant newMiddayPoint = TInstant::Seconds(time);
        return TInstant::Days(newMiddayPoint.Days()) + timeOfDay;
    #endif
    }

    TInstant ShiftCalendarMonths(const TInstant utc0, const i32 diff, const std::chrono::minutes utcOffset) {
        return ShiftCalendarMonths(utc0 + utcOffset, diff) - utcOffset;
    }

    TString FormatDuration(const TDuration duration) {
        if (duration < TDuration::Seconds(1)) {
            return ToString(duration.MilliSeconds()) + "ms";
        }
        if (duration.Seconds() % TDuration::Days(1).Seconds() == 0) {
            return ToString(duration.Days()) + "d";
        } else if (duration.Seconds() % TDuration::Hours(1).Seconds() == 0) {
            return ToString(duration.Hours()) + "h";
        } else if (duration.Seconds() % TDuration::Minutes(1).Seconds() == 0) {
            return ToString(duration.Minutes()) + "m";
        } else {
            return ToString(duration.Seconds()) + "s";
        }
    }

}
