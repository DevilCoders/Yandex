#pragma once

#include <util/system/defaults.h>

namespace ND2 {
namespace NImpl {
enum EPatternType {
    PT_UNKNOWN = 0,

    PT_T_DIG_JUNK,
    PT_U_DIG_JUNK,

    PT_T_DIG_TIME,
    PT_T_WRD_TIME,

    PT_H_YYYY,

    PT_U_ID,
    PT_U_ID_YYYYMMDD,
    PT_U_ID_AABBYYYY,

    PT_U_DIG_AABBYY,
    PT_U_DIG_AABBYYYY,
    PT_U_DIG_YYMMDD,
    PT_U_DIG_YYYYMMDD,
    PT_U_DIG_YYYYMM,
    PT_U_DIG_MMYYYY,
    PT_U_DIG_YYYY,

    PT_U_WRD_MMDDYY,
    PT_U_WRD_AAMMBB,
    PT_U_WRD_YYYYMMDD,
    PT_U_WRD_DDMMYYYY,
    PT_U_WRD_YYDDMM,
    PT_U_WRD_YYMM,
    PT_U_WRD_MMYY,

    PT_U_RNG_YYMMMM,
    PT_U_RNG_MMMMYY,

    PT_T_DIG_YYYYMMDD,
    PT_T_DIG_AABBYYYY,
    PT_T_DIG_AABBYY,
    PT_T_DIG_AABBTIME,
    PT_T_DIG_TIMEAABB,
    PT_T_DIG_YYYY,

    PT_T_WRD_DDMMYYYY,
    PT_T_WRD_MMDDYYYY,
    PT_T_WRD_YYYYDDMM,
    PT_T_WRD_YYYYMMDD,
    PT_T_WRD_DDMMYY,
    PT_T_WRD_MMYYYY,
    PT_T_WRD_YYYYMM,
    PT_T_WRD_DDMM,
    PT_T_WRD_MMDD,
    PT_T_WRD_MMYY,
    PT_T_WRD_YYMM,

    PT_T_RNG_DDMM_DDMMYYYY,
    PT_T_RNG_MMDD_MMDDYYYY,
    PT_T_RNG_DDMM_DDMM,
    PT_T_RNG_MMDD_MMDD,

    PT_T_RNG_MM_MMYYYY,
    PT_T_RNG_MM_MM,

    PT_T_RNG_YYYYMM_YYYYMM,
    PT_T_RNG_YYYYMM_YYYY,
    PT_T_RNG_YYYY_YYYYMM,

    PT_T_RNG_MMYYYY_MMYYYY,
    PT_T_RNG_MMYYYY_YYYY,
    PT_T_RNG_YYYY_MMYYYY,

    PT_T_RNG_DD_DDMMYYYY,
    PT_T_RNG_DD_DDMM,
};

struct TValidDateProfile {
    ui32 MinDateSpanCount;
    ui32 MaxDateSpanCount;

    ui32 MinDayCount;
    ui32 MaxDayCount;

    ui32 MinMonthCount;
    ui32 MaxMonthCount;

    ui32 MinYearCount;
    ui32 MaxYearCount;

    ui32 MinYear;
    ui32 MaxYear;

    ui32 MinYearToPast;
    ui32 MaxYearToFuture;

    bool IsTime;
    bool IsDateRange;
    bool IsDate;
    bool IsAmbiguous;
    bool IsId;
    bool MonthIsWord;

    static ui32 Year2ToYear4(ui32 year2, ui32 refyear) {
        return year2 > (refyear + 12)%100 ? year2 + 1900 : year2 + 2000;
    }

    bool GoodPattern(ui32 dspcnt, ui32 daycnt, ui32 moncnt, ui32 ycnt) const {
        return MinDateSpanCount <= dspcnt && dspcnt <= MaxDateSpanCount
            && MinDayCount <= daycnt && daycnt <= MaxDayCount
            && MinMonthCount <= moncnt && moncnt <= MaxMonthCount
            && MinYearCount <= ycnt && ycnt <= MaxYearCount;
    }

    bool GoodYear(ui32 year, ui32 refyear) const {
        return MinYear <= year && year <= MaxYear
            && refyear - MinYearToPast <= year && year <= refyear + MaxYearToFuture;
    }

    explicit TValidDateProfile(EPatternType pt);
};

}
}
