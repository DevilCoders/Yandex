#pragma once

#include <util/datetime/base.h>
#include <contrib/libs/cctz/include/cctz/time_zone.h>

namespace NUtil {
    /*
    More datetime libraries can be found here:
        library/cpp/timezone_conversion
        market/library/datetime/datetime.h
        search/web/util/datetime_formatter
        travel/rasp/route-search-api/datetime_helpers.h
        util/draft/datetime.h
        yabs/server/libs/moscow_time
    */

   // NB. Time zone names can be found here: https://en.wikipedia.org/wiki/List_of_tz_database_time_zones

    using TTimeZone = cctz::time_zone;

    static const TString utcTzName = "UTC";
    TTimeZone GetUTCTimeZone();

    bool GetTimeZone(const TString& name, TTimeZone& result);
    TTimeZone GetTimeZone(const TString& name);

    time_t ConvertTimeZone(const time_t timestamp, const TTimeZone& tzFrom, const TTimeZone& tzTo);
    TInstant ConvertTimeZone(const TInstant& absoluteTime, const TTimeZone& tzFrom, const TTimeZone& tzTo);

    TInstant ApplyTimeZone(const TInstant& absoluteTime, const TTimeZone& tz);
    bool ApplyTimeZone(const TInstant& absoluteTime, const TString& timezoneName, TInstant& result);

    bool ParseFomattedDatetime(const TString& value, const TString& format, tm& tm);

    TInstant ParseFomattedLocalDatetime(const TString& value, const TString& format, const TInstant& defaultValue = TInstant::Max());
    TInstant ParseFomattedLocalDatetime(const TString& value, const TString& format, const TString& timezoneName, const TInstant& defaultValue = TInstant::Max());

    TString FormatDatetime(const TInstant& absoluteTime, const TString& format);

    ui32 GetWeekDay(const TInstant& absoluteTime);
    ui32 GetDayMinutes(const TInstant& absoluteTime);
    double GetDayHours(const TInstant& absoluteTime);
    ui32 GetYearDay(const TInstant& absoluteTime, const bool forceLeapYear = false);  // forceLeapYear provided to unify year day over all years
}
