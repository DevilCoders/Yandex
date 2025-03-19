#pragma once

#include <util/datetime/base.h>

class TTimeZoneHelper {
public:
    TInstant Gmt(const TInstant instant) const;

    TInstant Local(const TInstant instant) const;

    TString FormatLocal(const TInstant instant, const char* format) const;
    TString FormatLocalDate(const TInstant instant) const;

    i64 Seconds() const;

    i64 Minutes() const;

    i64 Hours() const;

    static TTimeZoneHelper FromSeconds(const i64 offset);

    static TTimeZoneHelper FromMinutes(const i64 offset);

    static TTimeZoneHelper FromHours(const i64 offset);

    static TTimeZoneHelper Default();

private:
    TTimeZoneHelper(i64 offset);

    i64 Offset = 0;
};

class TLocalInstant {
public:
    TLocalInstant(const TInstant& instant, const TTimeZoneHelper& timeZone);

    bool operator==(const TLocalInstant& item) const {
        return Gmt() == item.Gmt();
    }

    TInstant Gmt() const;
    TString FormatLocalTime() const;

    ui8 DayOfWeek() const;

private:
    TInstant Local() const;

    TInstant Instant;
    TTimeZoneHelper TimeZone;
};

class TDateTimeParser {
public:
    static bool Parse(const TString& dateTime, const TString& dtTemplate, TInstant& result);
    static bool Parse(TStringBuf dateTime, const TString& dtTemplate, TInstant& result);
};


namespace NUtil {

    TInstant ShiftCalendarMonths(const TInstant i, const i32 m);

    // Returns UTC+0 timepoint, but performs calendar manipulation for `utcOffset` local time.
    // example: `const auto monthLaterByNewYorkCalendar = NUtil::ShiftCalendarMonths(timepointUTC0, 1, -4h);`
    TInstant ShiftCalendarMonths(const TInstant utc0, const i32 m, const std::chrono::minutes utcOffset);

    TString FormatDuration(const TDuration duration);
}
