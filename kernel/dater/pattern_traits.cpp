#include "pattern_traits.h"

#include <util/generic/utility.h>

namespace ND2 {
namespace NImpl {
template <EPatternType>
struct TDatePatternTraits;

template <ui32 Min, ui32 Max>
struct TDay {
    static const ui32 MinDayCount = Min;
    static const ui32 MaxDayCount = Max;
};

template <ui32 Min, ui32 Max>
struct TMonth {
    static const ui32 MinMonthCount = Min;
    static const ui32 MaxMonthCount = Max;
};

template <ui32 Min, ui32 Max>
struct TYear {
    static const ui32 MinYearCount = Min;
    static const ui32 MaxYearCount = Max;
};

template <ui32 Min, ui32 Max, ui32 RelMin, ui32 RelMax>
struct TValidYears {
    static const ui32 MinYear = Min;
    static const ui32 MaxYear = Max;

    static const ui32 MinYearToPast = RelMin;
    static const ui32 MaxYearToFuture = RelMax;
};

template <ui32 Min, ui32 Max>
struct TDateSpans {
    static const ui32 MinDateSpanCount = Min;
    static const ui32 MaxDateSpanCount = Max;
};

typedef TDateSpans<0, 0> TSpans0;
typedef TDateSpans<1, 1> TSpans1;
typedef TDateSpans<2, 2> TSpans2;
typedef TDateSpans<3, 3> TSpans3;
typedef TDateSpans<4, 4> TSpans4;
typedef TDateSpans<5, 5> TSpans5;

typedef TDay<0, 0> TNoDay;
typedef TDay<1, 1> THasDay;
typedef TDay<2, 2> TDaysRange;

typedef TMonth<0, 0> TNoMonth;
typedef TMonth<1, 1> THasMonth;
typedef TMonth<2, 2> TMonthsRange;

typedef TYear<0, 0> TNoYear;
typedef TYear<1, 1> THasYear;
typedef TYear<2, 2> TYearsRange;

typedef TValidYears<1990, 2199, 100, 12> TWWWYears;
typedef TValidYears<1999, 2199, 100, 2> TWWWYearsStrict;
typedef TValidYears<1000, 2199, 1100, 200> THistoryYears4;
typedef TValidYears<1900, 2199, 88, 12> THistoryYears;

template <> struct TDatePatternTraits <PT_T_DIG_TIME> : TSpans0, TNoDay, TNoMonth, TNoYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_WRD_TIME> : TSpans0, TNoDay, TNoMonth, TNoYear, TWWWYears {};

template <> struct TDatePatternTraits <PT_H_YYYY>       : TSpans1, TNoDay, TNoMonth, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_DIG_YYYY>   : TSpans1, TNoDay, TNoMonth, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_WRD_YYMM>   : TSpans2, TNoDay, THasMonth, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_WRD_MMYY>   : TSpans2, TNoDay, THasMonth, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_DIG_YYYYMM> : TSpans2, TNoDay, THasMonth, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_DIG_MMYYYY> : TSpans2, TNoDay, THasMonth, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_WRD_MMDDYY> : TSpans3, THasDay, THasMonth, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_WRD_YYDDMM> : TSpans3, THasDay, THasMonth, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_ID>         : TSpans0, TNoDay, TNoMonth, TNoYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_ID_YYYYMMDD>: TSpans3, THasDay, THasMonth, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_ID_AABBYYYY>: TSpans3, TDay<1,2>, TMonth<1,2>, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_RNG_YYMMMM> : TSpans3, TNoDay, TMonthsRange, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_RNG_MMMMYY> : TSpans3, TNoDay, TMonthsRange, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_DIG_YYMMDD>   : TDateSpans<1, 3>, TDay<0, 1>, TMonth<0, 1>, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_DIG_YYYYMMDD> : TDateSpans<1, 3>, TDay<0, 1>, TMonth<0, 1>, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_DIG_AABBYY>   : TSpans3, TDay<1, 2>, TMonth<1, 2>, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_DIG_AABBYYYY> : TSpans3, TDay<1, 2>, TMonth<1, 2>, THasYear, TWWWYearsStrict {};

template <> struct TDatePatternTraits <PT_U_WRD_AAMMBB>   : TSpans3, TDay<1, 2>, THasMonth, TYear<1, 2>, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_WRD_YYYYMMDD> : TDateSpans<2, 3>, TDay<0, 1>, THasMonth, THasYear, TWWWYearsStrict {};
template <> struct TDatePatternTraits <PT_U_WRD_DDMMYYYY> : TDateSpans<2, 3>, TDay<0, 1>, THasMonth, THasYear, TWWWYearsStrict {};

template <> struct TDatePatternTraits <PT_T_DIG_YYYYMMDD> : TSpans3, THasDay, THasMonth, THasYear, THistoryYears {};
template <> struct TDatePatternTraits <PT_T_DIG_AABBYYYY> : TSpans3, TDay<1, 2>, TMonth<1, 2>, THasYear, THistoryYears {};
template <> struct TDatePatternTraits <PT_T_DIG_AABBYY>   : TSpans3, TDay<1, 2>, TMonth<1, 2>, THasYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_DIG_AABBTIME> : TSpans2, TDay<1, 2>, TMonth<1, 2>, TNoYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_DIG_TIMEAABB> : TSpans2, TDay<1, 2>, TMonth<1, 2>, TNoYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_DIG_YYYY>     : TSpans1, TNoDay, TNoMonth, THasYear, TWWWYears {};

template <> struct TDatePatternTraits <PT_T_WRD_DDMMYY>   : TSpans3, THasDay, THasMonth, THasYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_WRD_DDMMYYYY> : TDateSpans<2, 3>, TDay<0, 1>, THasMonth, THasYear, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_WRD_MMDDYYYY> : TSpans3, THasDay, THasMonth, THasYear, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_WRD_YYYYDDMM> : TSpans3, THasDay, THasMonth, THasYear, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_WRD_YYYYMMDD> : TDateSpans<2, 3>, TDay<0, 1>, THasMonth, THasYear, THistoryYears4 {};

template <> struct TDatePatternTraits <PT_T_WRD_DDMM>     : TSpans2, THasDay, THasMonth, TNoYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_WRD_MMDD>     : TSpans2, THasDay, THasMonth, TNoYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_WRD_MMYY>     : TSpans2, TNoDay, THasMonth, THasYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_WRD_YYMM>     : TSpans2, TNoDay, THasMonth, THasYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_WRD_MMYYYY>   : TSpans2, TNoDay, THasMonth, THasYear, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_WRD_YYYYMM>   : TSpans2, TNoDay, THasMonth, THasYear, THistoryYears4 {};

template <> struct TDatePatternTraits <PT_T_RNG_DD_DDMM>       : TSpans3, TDaysRange, THasMonth, TNoYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_RNG_DD_DDMMYYYY>   : TSpans4, TDaysRange, THasMonth, THasYear, THistoryYears4 {};

template <> struct TDatePatternTraits <PT_T_RNG_DDMM_DDMM>     : TSpans4, TDaysRange, TMonthsRange, TNoYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_RNG_MMDD_MMDD>     : TSpans4, TDaysRange, TMonthsRange, TNoYear, TWWWYears {};
template <> struct TDatePatternTraits <PT_T_RNG_DDMM_DDMMYYYY> : TSpans5, TDaysRange, TMonthsRange, THasYear, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_RNG_MMDD_MMDDYYYY> : TSpans5, TDaysRange, TMonthsRange, THasYear, THistoryYears4 {};

template <> struct TDatePatternTraits <PT_T_RNG_MM_MM>         : TSpans2, TNoDay, TMonthsRange, TNoYear, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_RNG_MM_MMYYYY>     : TSpans3, TNoDay, TMonthsRange, THasYear, THistoryYears4 {};

template <> struct TDatePatternTraits <PT_T_RNG_MMYYYY_MMYYYY> : TSpans4, TNoDay, TMonthsRange, TYearsRange, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_RNG_YYYYMM_YYYYMM> : TSpans4, TNoDay, TMonthsRange, TYearsRange, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_RNG_YYYY_MMYYYY>   : TSpans3, TNoDay, THasMonth, TYearsRange, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_RNG_YYYY_YYYYMM>   : TSpans3, TNoDay, THasMonth, TYearsRange, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_RNG_MMYYYY_YYYY>   : TSpans3, TNoDay, THasMonth, TYearsRange, THistoryYears4 {};
template <> struct TDatePatternTraits <PT_T_RNG_YYYYMM_YYYY>   : TSpans3, TNoDay, THasMonth, TYearsRange, THistoryYears4 {};


template<EPatternType pt>
static void FillValidDateProfile(TValidDateProfile& p) {
    p.MinDateSpanCount = TDatePatternTraits<pt>::MinDateSpanCount;
    p.MaxDateSpanCount = TDatePatternTraits<pt>::MaxDateSpanCount;
    p.MinDayCount = TDatePatternTraits<pt>::MinDayCount;
    p.MaxDayCount = TDatePatternTraits<pt>::MaxDayCount;
    p.MinMonthCount = TDatePatternTraits<pt>::MinMonthCount;
    p.MaxMonthCount = TDatePatternTraits<pt>::MaxMonthCount;
    p.MinYearCount = TDatePatternTraits<pt>::MinYearCount;
    p.MaxYearCount = TDatePatternTraits<pt>::MaxYearCount;
    p.MinYear = TDatePatternTraits<pt>::MinYear;
    p.MaxYear = TDatePatternTraits<pt>::MaxYear;
    p.MinYearToPast = TDatePatternTraits<pt>::MinYearToPast;
    p.MaxYearToFuture = TDatePatternTraits<pt>::MaxYearToFuture;
}

TValidDateProfile::TValidDateProfile(EPatternType pt) {
    Zero(*this);
    IsDate = true;
    switch (pt) {
    default:
        break;

    case PT_U_ID:
        FillValidDateProfile<PT_U_ID> (*this);
        IsId = true;
        break;

    case PT_U_ID_YYYYMMDD:
        FillValidDateProfile<PT_U_ID_YYYYMMDD> (*this);
        IsId = true;
        break;

    case PT_U_ID_AABBYYYY:
        FillValidDateProfile<PT_U_ID_AABBYYYY> (*this);
        IsAmbiguous = true;
        IsId = true;
        break;

    case PT_T_DIG_TIME:
        FillValidDateProfile<PT_T_DIG_TIME> (*this);
        IsTime = true;
        IsDate = false;
        break;
    case PT_T_WRD_TIME:
        FillValidDateProfile<PT_T_WRD_TIME> (*this);
        IsTime = true;
        IsDate = false;
        break;

    case PT_H_YYYY:
        FillValidDateProfile<PT_H_YYYY> (*this);
        break;

    case PT_U_DIG_AABBYY:
        FillValidDateProfile<PT_U_DIG_AABBYY> (*this);
        IsAmbiguous = true;
        break;
    case PT_U_DIG_AABBYYYY:
        FillValidDateProfile<PT_U_DIG_AABBYYYY> (*this);
        IsAmbiguous = true;
        break;
    case PT_U_DIG_YYMMDD:
        FillValidDateProfile<PT_U_DIG_YYMMDD> (*this);
        break;
    case PT_U_DIG_YYYYMMDD:
        FillValidDateProfile<PT_U_DIG_YYYYMMDD> (*this);
        break;

    case PT_U_DIG_YYYYMM:
        FillValidDateProfile<PT_U_DIG_YYYYMM> (*this);
        break;
    case PT_U_DIG_MMYYYY:
        FillValidDateProfile<PT_U_DIG_MMYYYY> (*this);
        break;
    case PT_U_DIG_YYYY:
        FillValidDateProfile<PT_U_DIG_YYYY> (*this);
        break;

    case PT_U_WRD_MMDDYY:
        FillValidDateProfile<PT_U_WRD_MMDDYY> (*this);
        MonthIsWord = true;
        break;
    case PT_U_WRD_AAMMBB:
        FillValidDateProfile<PT_U_WRD_AAMMBB> (*this);
        IsAmbiguous = true;
        MonthIsWord = true;
        break;

    case PT_U_WRD_DDMMYYYY:
        FillValidDateProfile<PT_U_WRD_DDMMYYYY> (*this);
        MonthIsWord = true;
        break;
    case PT_U_WRD_YYYYMMDD:
        FillValidDateProfile<PT_U_WRD_YYYYMMDD> (*this);
        MonthIsWord = true;
        break;

    case PT_U_WRD_YYDDMM:
        FillValidDateProfile<PT_U_WRD_YYDDMM> (*this);
        MonthIsWord = true;
        break;
    case PT_U_WRD_YYMM:
        FillValidDateProfile<PT_U_WRD_YYMM> (*this);
        MonthIsWord = true;
        break;
    case PT_U_WRD_MMYY:
        FillValidDateProfile<PT_U_WRD_MMYY> (*this);
        MonthIsWord = true;
        break;

    case PT_U_RNG_YYMMMM:
        FillValidDateProfile<PT_U_RNG_YYMMMM> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_U_RNG_MMMMYY:
        FillValidDateProfile<PT_U_RNG_MMMMYY> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;

    case PT_T_DIG_YYYYMMDD:
        FillValidDateProfile<PT_T_DIG_YYYYMMDD> (*this);
        break;
    case PT_T_DIG_AABBYYYY:
        FillValidDateProfile<PT_T_DIG_AABBYYYY> (*this);
        IsAmbiguous = true;
        break;
    case PT_T_DIG_AABBYY:
        FillValidDateProfile<PT_T_DIG_AABBYY> (*this);
        IsAmbiguous = true;
        break;
    case PT_T_DIG_AABBTIME:
        FillValidDateProfile<PT_T_DIG_AABBTIME> (*this);
        IsAmbiguous = true;
        break;
    case PT_T_DIG_TIMEAABB:
        FillValidDateProfile<PT_T_DIG_TIMEAABB> (*this);
        IsAmbiguous = true;
        break;
    case PT_T_DIG_YYYY:
        FillValidDateProfile<PT_T_DIG_YYYY> (*this);
        break;

    case PT_T_WRD_DDMMYYYY:
        FillValidDateProfile<PT_T_WRD_DDMMYYYY> (*this);
        MonthIsWord = true;
        break;
    case PT_T_WRD_MMDDYYYY:
        FillValidDateProfile<PT_T_WRD_MMDDYYYY> (*this);
        MonthIsWord = true;
        break;
    case PT_T_WRD_YYYYDDMM:
        FillValidDateProfile<PT_T_WRD_YYYYDDMM> (*this);
        MonthIsWord = true;
        break;
    case PT_T_WRD_YYYYMMDD:
        FillValidDateProfile<PT_T_WRD_YYYYMMDD> (*this);
        MonthIsWord = true;
        break;
    case PT_T_WRD_DDMMYY:
        FillValidDateProfile<PT_T_WRD_DDMMYY> (*this);
        MonthIsWord = true;
        break;
    case PT_T_WRD_MMYYYY:
        FillValidDateProfile<PT_T_WRD_MMYYYY> (*this);
        MonthIsWord = true;
        break;
    case PT_T_WRD_YYYYMM:
        FillValidDateProfile<PT_T_WRD_YYYYMM> (*this);
        MonthIsWord = true;
        break;
    case PT_T_WRD_DDMM:
        FillValidDateProfile<PT_T_WRD_DDMM> (*this);
        MonthIsWord = true;
        break;
    case PT_T_WRD_MMDD:
        FillValidDateProfile<PT_T_WRD_MMDD> (*this);
        MonthIsWord = true;
        break;
    case PT_T_WRD_MMYY:
        FillValidDateProfile<PT_T_WRD_MMYY> (*this);
        MonthIsWord = true;
        break;
    case PT_T_WRD_YYMM:
        FillValidDateProfile<PT_T_WRD_YYMM> (*this);
        MonthIsWord = true;
        break;

    case PT_T_RNG_DDMM_DDMMYYYY:
        FillValidDateProfile<PT_T_RNG_DDMM_DDMMYYYY> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_T_RNG_DDMM_DDMM:
        FillValidDateProfile<PT_T_RNG_DDMM_DDMM> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_T_RNG_MMDD_MMDDYYYY:
        FillValidDateProfile<PT_T_RNG_MMDD_MMDDYYYY> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_T_RNG_MMDD_MMDD:
        FillValidDateProfile<PT_T_RNG_MMDD_MMDD> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_T_RNG_MM_MMYYYY:
        FillValidDateProfile<PT_T_RNG_MM_MMYYYY> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_T_RNG_MM_MM:
        FillValidDateProfile<PT_T_RNG_MM_MM> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_T_RNG_DD_DDMMYYYY:
        FillValidDateProfile<PT_T_RNG_DD_DDMMYYYY> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;

    case PT_T_RNG_MMYYYY_MMYYYY:
        FillValidDateProfile<PT_T_RNG_MMYYYY_MMYYYY> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_T_RNG_YYYYMM_YYYYMM:
        FillValidDateProfile<PT_T_RNG_YYYYMM_YYYYMM> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;

    case PT_T_RNG_YYYY_MMYYYY:
        FillValidDateProfile<PT_T_RNG_YYYY_MMYYYY> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_T_RNG_YYYY_YYYYMM:
        FillValidDateProfile<PT_T_RNG_YYYY_YYYYMM> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_T_RNG_MMYYYY_YYYY:
        FillValidDateProfile<PT_T_RNG_MMYYYY_YYYY> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    case PT_T_RNG_YYYYMM_YYYY:
        FillValidDateProfile<PT_T_RNG_YYYYMM_YYYY> (*this);
        IsDateRange = true;
        MonthIsWord = true;
        break;
    }
}

}
}
