#pragma once

#include <util/generic/string.h>

namespace NIPREG {
    struct TAddress;
};

namespace NIpregTest {
    TString GetFullAddr(const TString& addr);

    TString BuildRange(size_t start, size_t end);
    TString BuildRange(const TString& start, const TString& end);

    TString BuildFullRangeRow(size_t start, size_t end, const TString& data);
    TString BuildFullRangeRow(const TString& start, const TString& end, const TString& data);
    TString BuildFullRangeRow(const NIPREG::TAddress& start, const NIPREG::TAddress& end, const TString& data);

    size_t DetectLinesAmount(const TString& input);

    extern const TString EMPTY;
    extern const TString SEP_HYP;
    extern const TString SEP_TAB;
    extern const TString ROW_END;

    extern const TString EMPTY_IPREG_DATA;

    extern const TString IPREG_DATA_1;
    extern const TString IPREG_DATA_2;
    extern const TString IPREG_DATA_3;

    extern const TString IPREG_UNSORT_REAL_DATA_1;
    extern const TString IPREG_UNSORT_REAL_DATA_2;
    extern const TString IPREG_UNSORT_REAL_DATA_3;

    extern const TString IPREG_SORTED_REAL_DATA;

    extern const TString PATCH_DATA_1;
    extern const TString PATCH_DATA_2;

    extern const TString IPREG_DATA_1_PATCH_1;
    extern const TString IPREG_DATA_2_PATCH_1;
    extern const TString IPREG_DATA_3_PATCH_1;

    extern const TString IPREG_DATA_1_PATCH_2;
    extern const TString IPREG_DATA_2_PATCH_2;
    extern const TString IPREG_DATA_3_PATCH_2;

    extern const TString SOME_RANGES_DATA;

    extern const TString RANGE_1_5;
    extern const TString RANGE_6_9;

    extern const TString RANGE_1_7;
    extern const TString RANGE_5_9;

    extern const TString RANGE_1_9;
    extern const TString RANGE_4_5;

    extern const TString RANGE_1_1;
    extern const TString RANGE_5_5;
    extern const TString RANGE_9_9;

    extern const TString IPV6_FIRST;
    extern const TString IPV6_LAST;
} // NIpregTest
