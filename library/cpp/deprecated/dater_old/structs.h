#pragma once

#include <cstdio>

#include <util/system/yassert.h>
#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/list.h>
#include <util/generic/vector.h>
#include <util/draft/datetime.h>

#include <library/cpp/deprecated/dater_old/date_attr_def/date_attr_def.h>

namespace NDater {
    struct TDaterDate {
        enum EDateFrom {
            // erf.DaterFrom
            FromUnknown = 0,
            FromContent = 1,            // content segment
            FromTitle = 2,              // title tag
            FromUrl = 3,                // everything inside Url which cannot go in other categories
            FromExternal = 4,           // feeds etc. no longer used in production.
            FromText = 5,               // everything inside Document which cannot go in other categories
            FromFooter = 6,             // footer segment
            FromMainContent = 7,        // main content zone
            FromUrlId = 8,              // date-based identificators in urls
            FromHost = 9,               // years in hosts
            FromBeforeMainContent = 10, // ~ the segment just before main content
            FromAfterMainContent = 11,  // ~ the segment just after main content
            FromMainContentStart = 12,  // ~ the first segment of main content
            FromMainContentEnd = 13,    // ~ the last  segment of main content
            // how many sources we have now
            FromsCount = 14,
        };

        enum ETrustLevel {
            TrustLevelNone = 0,
            TrustLevelFullNoYear,
            TrustLevelFooterYO,
            TrustLevelTextYO,
            TrustLevelMainContentYO,
            TrustLevelMainContentEndsYO,
            TrustLevelAroundMainContentYO,
            TrustLevelTitleYO,
            TrustLevelUrlYO,
            TrustLevelHostYO,
            TrustLevelFooter,
            TrustLevelText,
            TrustLevelMainContent,
            TrustLevelMainContentEnds,
            TrustLevelAroundMainContent,
            TrustLevelTitle,
            TrustLevelUrl,
            TrustLevelExternal
        };

        enum EDateMode {
            ModeFull = 0,
            ModeNoDay = 1,
            ModeNoMonth = 3,
            ModeFullNoYear = 4,
            ModesCount = 5,
            ModeInvalid = -1
        };

        enum {
            ErfZeroYear = 1989,
            ErfMinYear = ErfZeroYear + 1,
            ErfMaxYear = 2020,

            MaxYear = 2101,
            MinYear = -8191,
            FirstDayOrMonth = 1,

            MidYear = 183,
            MidMonth = 16,
        };

        union {
            ui32 All;

            struct {
                ui32 Year : 13; // 1 - 8191, 0 - date unknown entirely
                ui32 Month : 4; // 1 - 12, 0 - unknown
                ui32 Day : 5;   // 1 - 31, 0 - unknown

                ui32 From : 4;
                ui32 LongYear : 1;
                ui32 WordPattern : 1;
                ui32 NoYear : 1;
                ui32 Unused : 3;
            };
        };

    public:
        static const size_t INDEX_ATTR_BUFSIZE = 4 /* year */ + 2 /* month */ + 2 /* day */ + 1 /* '\0' */;

    public:
        explicit TDaterDate(ui32 year = 0, ui32 month = 0, ui32 day = 0, EDateFrom from = FromUnknown,
                            bool longyear = false, bool wordpattern = false, bool noyear = false)
            //: All() // this makes gcc 4.6 happy
            : Year(year)
            , Month(month)
            , Day(day)
            , From(from)
            , LongYear(longyear)
            , WordPattern(wordpattern)
            , NoYear(noyear)
            , Unused(0)
        {
        }

        static TDaterDate MakeDate(ui32 year, ui32 month, ui32 day, EDateMode mode, EDateFrom from,
                                   bool word, bool longyear, bool normyear) {
            if (normyear)
                NormalizeYear(year);

            if (!Validate(mode, year, month, day))
                return TDaterDate();

            NormalizeDay(day, month, year);
            return TDaterDate(year, month, day, from, longyear, word, mode == ModeFullNoYear);
        }

        static TDaterDate MakeDateFull(ui32 year, ui32 month, ui32 day, EDateFrom from = FromUnknown,
                                       bool word = false, bool normyear = true) {
            return MakeDate(year, month, day, ModeFull, from, word, year > 1000, normyear);
        }

        static TDaterDate MakeDateMonth(ui32 year, ui32 mon, EDateFrom from = FromUnknown, bool word = false, bool normyear = true) {
            return MakeDate(year, mon, 0, ModeNoDay, from, word, year > 1000, normyear);
        }

        static TDaterDate MakeDateYear(ui32 year, EDateFrom from = FromUnknown, bool normyear = true) {
            return MakeDate(year, 0, 0, ModeNoMonth, from, false, year > 1000, normyear);
        }

        static TDaterDate MakeDateFullNoYear(ui32 month, ui32 day, EDateFrom from = FromUnknown, bool word = false) {
            return MakeDate(0, month, day, ModeFullNoYear, from, word, false, false);
        }

        static TDaterDate FromIndexAttr(char* attr);

        bool IsFromUrl() const {
            return From == FromUrl || From == FromUrlId;
        }

        bool IsFromGoodText() const {
            return !IsFromUrl() && IsVeryTrusted();
        }

        bool YearOnly() const {
            return Year && !Month;
        }

        EDateMode GetMode() const {
            if (NoYear && Month && Day)
                return ModeFullNoYear;
            return Year ? Month ? Day ? ModeFull : ModeNoDay : ModeNoMonth : ModeInvalid;
        }

        bool Valid() const {
            return Year || NoYear && Month && Day;
        }

        bool Defined() const {
            return SaneInternetDate() && From > FromUnknown && From < FromsCount;
        }

        bool SaneInternetDate() const {
            return Valid() && (Year >= ErfMinYear && Year <= ErfMaxYear || NoYear && Month && Day);
        }

        bool IsVeryTrusted() const {
            return TrustLevel() >= TrustLevelAroundMainContent;
        }

        bool IsMoreTrusted() const {
            return TrustLevel() >= TrustLevelMainContentEnds;
        }

        bool IsTrusted() const {
            return TrustLevel() >= TrustLevelMainContent;
        }

        bool IsLessTrusted() const {
            return TrustLevel() >= TrustLevelText;
        }

        ui32 TrustLevel() const {
            if (!SaneInternetDate())
                return TrustLevelNone;

            if (NoYear)
                return TrustLevelFullNoYear;

            switch (From) {
                case FromExternal:
                    return TrustLevelExternal;
                case FromHost:
                    return TrustLevelHostYO;
                case FromUrl:
                case FromUrlId:
                    return YearOnly() ? TrustLevelUrlYO : TrustLevelUrl;
                case FromTitle:
                    return YearOnly() ? TrustLevelTitleYO : TrustLevelTitle;
                case FromBeforeMainContent:
                case FromAfterMainContent:
                    return YearOnly() ? TrustLevelAroundMainContentYO : TrustLevelAroundMainContent;
                case FromMainContentStart:
                case FromMainContentEnd:
                    return YearOnly() ? TrustLevelMainContentEndsYO : TrustLevelMainContentEnds;
                case FromMainContent:
                    return YearOnly() ? TrustLevelMainContentYO : TrustLevelMainContent;
                case FromContent:
                case FromText:
                    return YearOnly() ? TrustLevelTextYO : TrustLevelText;
                case FromFooter:
                    return YearOnly() ? TrustLevelFooterYO : TrustLevelFooter;
            }
            return TrustLevelNone;
        }

        NDatetime::TSimpleTM ToSimpleTM() const {
            NDatetime::TSimpleTM tm = NDatetime::TSimpleTM::New();

            if (!Valid())
                return tm;

            return tm.SetRealDate(Year, Month, Day);
        }

        time_t ToTimeT() const {
            if (!Valid() || Year < 1970)
                return 0;

            return ToSimpleTM().AsTimeT();
        }

        // serves as a documentation and default help string provider
        static const char* FormatHelp() {
            return "Supports the following formatters and their combinations:\n"
                   "\t%d day (00-31),\n"
                   "\t%m month (00-12),\n"
                   "\t%Y year (e.g. 0000-3000),\n"
                   "\t%f source one-char code,\n"
                   "\t%F source description,\n"
                   "\t%p pattern one-char code,\n"
                   "\t%P pattern description,\n"
                   "\t%t time_t representation.\n"
                   "The following formatters will result in either number or empty string"
                   " depending on all the dates being defined:\n"
                   "\t%g distance in days from the given date,\n"
                   "\t%G distance in months from the given date,\n"
                   "\t%K distance in years from the given date.\n"
                   "No percent sign escape is provided. See also strftime from <ctime>.\n";
        }

        // Formats the date (see FormatHelp).
        TString ToString(TString format = DefaultDateFormat(), TDaterDate refDate = TDaterDate()) const;

        char* ToIndexAttr(char* buffer) const;

        bool CanDiff(TDaterDate refDate) const {
            return Valid() && refDate.Valid() && !NoYear && !refDate.NoYear;
        }

        i32 DiffDays(TDaterDate refDate) const {
            if (!CanDiff(refDate))
                return refDate ? Max<i32>() : Min<i32>();
            return GetDaysAD() - refDate.GetDaysAD();
        }

        i32 DiffMonths(TDaterDate refDate) const {
            if (!CanDiff(refDate))
                return DiffDays(refDate);
            if (!Month || !refDate.Month)
                return DiffYears(refDate) * 12;
            return GetMonthsAD() - refDate.GetMonthsAD();
        }

        i32 DiffYears(TDaterDate refDate) const {
            if (!CanDiff(refDate))
                return refDate ? Max<i32>() : Min<i32>();
            return Year - refDate.Year;
        }

        /*Gives the count of months A.D.*/
        i32 GetMonthsAD() const {
            i32 year = Year;
            return Year ? (year - 1) * 12 + (Month ? Month : 6) : 0;
        }

        /*Gives the count of days A.D.*/
        i32 GetDaysAD() const {
            if (!Valid() || !Year)
                return 0;

            bool leap = NDatetime::LeapYearAD(Year);
            ui32 monidx = Max<ui32>(Month, 1) - 1;
            ui32 daysnewyear = NDatetime::MonthDaysNewYear[leap][monidx];
            i32 daysad = NDatetime::YearDaysAD(Year);

            return (Day ? Day : NDatetime::MonthDays[leap][monidx] / 2) + (Month ? daysnewyear : (365 + leap) / 2) + daysad;
        }

        operator bool() const {
            return Valid();
        }

        bool friend SameButDay(const TDaterDate& a, const TDaterDate& b) {
            return a.Year == b.Year && a.Month == b.Month;
        }

        bool friend operator==(const TDaterDate& a, const TDaterDate& b) {
            return SameButDay(a, b) && a.Day == b.Day;
        }

        bool friend operator<(const TDaterDate& a, const TDaterDate& b) {
            return a.Year < b.Year || a.Year == b.Year && (a.Month < b.Month || a.Month == b.Month && a.Day < b.Day);
        }

        bool friend operator!=(const TDaterDate& a, const TDaterDate& b) {
            return !(a == b);
        }

        bool friend operator<=(const TDaterDate& a, const TDaterDate& b) {
            return !(a > b);
        }

        bool friend operator>(const TDaterDate& a, const TDaterDate& b) {
            return b < a;
        }

        bool friend operator>=(const TDaterDate& a, const TDaterDate& b) {
            return !(a < b);
        }

    public:
        static EDateMode GetMode(ui32 day, ui32 month, ui32 year) {
            if (!year && day && month)
                return ModeFullNoYear;
            return day ? ModeFull : month ? ModeNoDay : ModeNoMonth;
        }

        static const char* DefaultDateFormat() {
            return "%d/%m/%Y@%f.%p";
        }

        static size_t DefaultFormattedDateLen() {
            return 14; // "dd/mm/YYYY@f.d" = 3 + 3 + 5 + 3
        }

        static TDaterDate Now() {
            return FromTimeT(time(nullptr));
        }

        /*formatted read is not implemented yet*/
        static TDaterDate FromString(const TString& s);
        static TDaterDate FromSimpleTM(const NDatetime::TSimpleTM& tm, ui16 from = FromExternal);
        static TDaterDate FromTimeT(time_t t, ui16 from = FromExternal);
        static TDaterDate FromTM(const struct tm& tinfo, ui16 from = FromExternal);
        static TDaterDate FromTM(ui16 tm_mday, ui16 tm_mon, ui16 tm_year, ui16 from = FromExternal);

        static const char* EncodeFrom(ui32 from, bool full = false);
        static EDateFrom DecodeFrom(char code);

        static TString EncodePattern(const TDaterDate& d, bool full = false);
        static bool IsWordPattern(char code);
        static bool IsLongYear(char code);

        static bool Validate(EDateMode mode, ui32 year, ui32 month, ui32 day) {
            if (!year && mode != ModeFullNoYear || !(mode & ModeNoMonth) && !month || !(mode & ModeNoDay) && !day)
                return false;

            if (mode == ModeFullNoYear && !month && !day)
                return false;

            if (year > (ui32)MaxYear || month > 12 || day > ui32(29 + 2 * (month != 2)))
                return false;

            return true;
        }

        static bool NormalizeYear(ui32& year) {
            if (!year) {
                year = 2000;
                return false;
            }

            if (year > 0 && year < 20) {
                year += 2000;
                return false;
            }

            if (year > 0 && year < 100) {
                year += 1900;
                return false;
            }

            return true;
        }

        static void NormalizeDay(ui32& day, ui32& month, ui32 year) {
            if (!month) {
                day = 0;
                return;
            }

            if (month > 12u) {
                month = 0;
                return;
            }

            ui32 mdays = NDatetime::MonthDays[NDatetime::LeapYearAD(year)][Min(month, 12u) - 1];
            if (day > mdays) { // day cannot be > 31
                ++month;
                day -= mdays;
            }

            if (day > mdays) {
                day = 0;
                month = 0;
                return;
            }
        }
    };

    template <typename TErfFormat>
    void WriteBestDateToErf(const TDaterDate& date, TErfFormat& erf) {
        erf.DaterYear = date.Year ? date.Year - TDaterDate::ErfZeroYear : 0;
        erf.DaterMonth = date.Month;
        erf.DaterDay = date.Day;
        erf.DaterFrom = date.From & 7;
        erf.DaterFrom1 = (date.From >> 3) & 1;
    }

    template <typename TErfFormat>
    TDaterDate ReadBestDateFromErf(const TErfFormat& erf) {
        ui32 from = erf.DaterFrom | (erf.DaterFrom1 << 3);

        if (erf.DaterYear)
            return TDaterDate(erf.DaterYear + TDaterDate::ErfZeroYear, erf.DaterMonth, erf.DaterDay, (TDaterDate::EDateFrom)from);
        else if (erf.DaterMonth && erf.DaterDay)
            return TDaterDate::MakeDateFullNoYear(erf.DaterMonth, erf.DaterDay, (TDaterDate::EDateFrom)from, false);
        else
            return TDaterDate();
    }

    typedef TVector<TDaterDate> TDaterDates;

}
