#include <util/stream/file.h>

#include <library/cpp/deprecated/split/delim_string_iter.h>

#include "is_holiday.h"

size_t THolidayChecker::TRegionDateHash::operator()(const TRegionDate& regionDate) const
{
    return THash<time_t>()(regionDate.second.GetStart() + regionDate.first);
}

bool THolidayChecker::IsWeekend(const TDate& date)
{
    const unsigned weekDay = date.GetWeekDay();
    return (weekDay == 0 || weekDay == 6);
}

void THolidayChecker::AddRule(TGeoRegion region, bool holiday, const TDate& date)
{
    THolidaysHash& targetHash = holiday ? Holidays : Workdays;
    targetHash.insert(std::make_pair(region, date));
}

THolidayChecker::THolidayChecker(const TString& holidayPath, const TRegionsDB* regionsDB)
    : RegionsDB(regionsDB)
{
    if (holidayPath.length() == 0) {
        return;
    }
    TFileInput rrIn(holidayPath);
    TString lineStr;
    TString regionStr;
    TString dateStr;
    unsigned line = 0;
    while (rrIn.ReadTo(lineStr, '\n')) {
        ++line;
        if (lineStr[0] == '#') { // uses the fact that TString is c-str
            continue;
        }
        TDelimStringIter pIt(lineStr, "\t");
        pIt.Next(regionStr);
        TGeoRegion region = FromString<TGeoRegion>(regionStr);
        if (!pIt.TryNext(dateStr)) {
            ythrow yexception() << "can't read date at line " << line;
        }
        TDate date(TString(dateStr.begin() + 1, dateStr.end()).c_str());
        const char& specifier = dateStr[0];
        switch (specifier) {
            case '+':
                AddRule(region, true, date);
                break;
            case '-':
                AddRule(region, false, date);
                break;
            default:
                ythrow yexception() << "unexpected specifier before date: '" << specifier << "', '+' or '-' expected";
        }
    }
}

bool THolidayChecker::IsHoliday(const TDate& date, TGeoRegion region) const
{
    while (region != 0) {
        const TRegionDate key = std::make_pair(region, date);
        if (Holidays.contains(key)) {
            return true;
        }
        if (Workdays.contains(key)) {
            return false;
        }
        region = RegionsDB == nullptr ? 0 : RegionsDB->GetParent(region);
    }
    return IsWeekend(date);
}
