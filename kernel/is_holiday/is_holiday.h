#pragma once

#include <time.h>

#include <util/ysaveload.h>
#include <util/draft/date.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/datetime/constants.h>
#include <util/datetime/systime.h>
#include <util/stream/output.h>
#include <util/stream/mem.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <utility>

#include <kernel/geo/utils.h>


class THolidayChecker
{

private:
    typedef std::pair<TGeoRegion, TDate> TRegionDate;

    struct TRegionDateHash {
        size_t operator()(const TRegionDate& regionDate) const;
    };

    typedef THashSet<TRegionDate, TRegionDateHash> THolidaysHash;

    THolidaysHash Holidays;
    THolidaysHash Workdays;
    const TRegionsDB* const RegionsDB;

private:
    static bool IsWeekend(const TDate& date);
    void AddRule(TGeoRegion region, bool holiday, const TDate& date);

public:
    THolidayChecker(const TString& holidayPath, const TRegionsDB* regionsDB = nullptr);

    bool IsHoliday(const TDate& date, TGeoRegion region = 225) const;

    template <typename TParentIter>
    bool IsHoliday(const TDate& date, TParentIter iter) const {
        while (iter.IsValid()) {
            const TRegionDate key = std::make_pair(iter.Get(), date);
            if (Holidays.contains(key)) {
                return true;
            }
            if (Workdays.contains(key)) {
                return false;
            }
            iter.Next();
        }
        return IsWeekend(date);
    }

};


