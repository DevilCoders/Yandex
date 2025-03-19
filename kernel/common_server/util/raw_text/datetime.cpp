#include <kernel/common_server/util/raw_text/datetime.h>

#include <util/datetime/constants.h>

namespace NUtil {
    using TSystemClock = std::chrono::system_clock;

    TTimeZone GetUTCTimeZone() {
        return cctz::utc_time_zone();
    }

    bool GetTimeZone(const TString& name, TTimeZone& result) {
        return cctz::load_time_zone(static_cast<std::string>(name), &result);
    }

    TTimeZone GetTimeZone(const TString& name) {
        TTimeZone result;
        Y_ENSURE(!!GetTimeZone(name, result), "timezone not found: " + name);
        return result;
    }

    time_t ConvertTimeZone(const time_t timestamp, const TTimeZone& tzFrom, const TTimeZone& tzTo) {
        auto tp = TSystemClock::from_time_t(timestamp);
        auto cs = cctz::convert(tp, tzTo);
        return TSystemClock::to_time_t(tzFrom.lookup(cs).pre);
    }

    TInstant ConvertTimeZone(const TInstant& absoluteTime, const TTimeZone& tzFrom, const TTimeZone& tzTo) {
        return TInstant::Seconds(ConvertTimeZone(absoluteTime.TimeT(), tzFrom, tzTo));
    }

    TInstant ApplyTimeZone(const TInstant& absoluteTime, const TTimeZone& tz) {
        // treat timestamp as time local to timezone
        return ConvertTimeZone(absoluteTime, tz, cctz::utc_time_zone());
    }

    bool ApplyTimeZone(const TInstant& absoluteTime, const TString& timezoneName, TInstant& result) {
        TTimeZone tz;
        if (!GetTimeZone(timezoneName, tz)) {
            return false;
        }

        result = ApplyTimeZone(absoluteTime, tz);
        return true;
    }

    bool ParseFomattedDatetime(const TString& value, const TString& format, tm& tm) {
        Zero(tm);  // is_dst is not set
        try {
            return strptime(value.data(), format.data(), &tm) != nullptr;
        } catch (...) {
            return false;
        }
    }

    TInstant ParseFomattedLocalDatetime(const TString& value, const TString& format, const TInstant& defaultValue) {
        return ParseFomattedLocalDatetime(value, format, utcTzName, defaultValue);
    }

    TInstant ParseFomattedLocalDatetime(const TString& value, const TString& format, const TString& timezoneName, const TInstant& defaultValue) {
        TInstant result;

        tm tm;
        TTimeZone tz;

        try {
            if (ParseFomattedDatetime(value, format, tm) && GetTimeZone(timezoneName, tz)) {
                result = ApplyTimeZone(TInstant::Seconds(timegm(&tm)), tz);
            } else {
                result = defaultValue;
            }
        } catch (...) {
            result = defaultValue;
        }

        return result;
    }

    TString FormatDatetime(const TInstant& absoluteTime, const TString& format) {
        tm t;
        return Strftime(format.data(), absoluteTime.GmTime(&t));
    }

    ui32 GetWeekDay(const TInstant& absoluteTime) {
        tm t;
        absoluteTime.GmTime(&t);
        return static_cast<ui32>((t.tm_wday + 6) % 7);  // 0 - Monday, ..., 6 - Sunday
    }

    ui32 GetDayMinutes(const TInstant& absoluteTime) {
        tm t;
        absoluteTime.GmTime(&t);
        return static_cast<ui32>(t.tm_hour * 60 + t.tm_min);
    }

    double GetDayHours(const TInstant& absoluteTime) {
        return 24 * (absoluteTime.Seconds() - SECONDS_IN_DAY * absoluteTime.Days()) / static_cast<double>(SECONDS_IN_DAY);
    }

    ui32 GetYearDay(const TInstant& absoluteTime, const bool forceLeapYear) {
        tm t;
        absoluteTime.GmTime(&t);
        ui32 yearDay = static_cast<ui32>(t.tm_yday);  // 0 - January 1, ..., 364 (365) - December 31
        if (forceLeapYear) {
            const bool requireExtra29Feb = (!cctz::detail::impl::is_leap_year(1900 + t.tm_year) && t.tm_mon >= 2);
            if (requireExtra29Feb) {
                ++yearDay;
            }
        }
        return yearDay;
    }
}
