#include "test_helpers.hpp"

#include <library/cpp/ipreg/address.h>

namespace NIpregTest {
    using namespace NIPREG;

    const TString EMPTY{};

    const TString SEP_HYP{"-"};
    const TString SEP_TAB{"\t"};
    const TString ROW_END{"\n"};

    const TString ipregData1 = "{\"ipreg\":1}";
    const TString ipregData2 = "{\"ipreg\":2}";
    const TString ipregData3 = "{\"ipreg\":3}";

    const TString EMPTY_IPREG_DATA = "{}" + ROW_END;

    const TString IPREG_DATA_1 = ipregData1 + ROW_END;
    const TString IPREG_DATA_2 = ipregData2 + ROW_END;
    const TString IPREG_DATA_3 = ipregData3 + ROW_END;

    const TString IPREG_UNSORT_REAL_DATA_1 = "{\"reliability\":10.1,\"is_placeholder\":0,\"region_id\":225}" + ROW_END;
    const TString IPREG_UNSORT_REAL_DATA_2 = "{\"reliability\":10.1,\"region_id\":225,\"is_placeholder\":0}" + ROW_END;
    const TString IPREG_UNSORT_REAL_DATA_3 = "{\"is_placeholder\":0,\"reliability\":10.1,\"region_id\":225}" + ROW_END;

    const TString IPREG_SORTED_REAL_DATA = "{\"is_placeholder\":0,\"region_id\":225,\"reliability\":10.1}" + ROW_END;

    const TString patchData1 = "{\"patch\":1}";
    const TString patchData2 = "{\"patch\":2}";

    const TString PATCH_DATA_1 = patchData1 + ROW_END;
    const TString PATCH_DATA_2 = patchData2 + ROW_END;

    const TString IPREG_DATA_1_PATCH_1 = "{\"patch\":1,\"ipreg\":1}" + ROW_END;
    const TString IPREG_DATA_2_PATCH_1 = "{\"patch\":1,\"ipreg\":2}" + ROW_END;
    const TString IPREG_DATA_3_PATCH_1 = "{\"patch\":1,\"ipreg\":3}" + ROW_END;

    const TString IPREG_DATA_1_PATCH_2 = "{\"patch\":2,\"ipreg\":1}" + ROW_END;
    const TString IPREG_DATA_2_PATCH_2 = "{\"patch\":2,\"ipreg\":2}" + ROW_END;
    const TString IPREG_DATA_3_PATCH_2 = "{\"patch\":2,\"ipreg\":3}" + ROW_END;

    const TString SOME_RANGES_DATA =
        BuildFullRangeRow(0, 1, IPREG_DATA_1) +
        BuildFullRangeRow(2, 4, IPREG_DATA_2) +
        BuildFullRangeRow(5, 9, IPREG_DATA_3);

    const TString RANGE_1_5 = BuildRange(1, 5);
    const TString RANGE_6_9 = BuildRange(6, 9);

    const TString RANGE_1_7 = BuildRange(1, 7);
    const TString RANGE_5_9 = BuildRange(5, 9);

    const TString RANGE_1_9 = BuildRange(1, 9);
    const TString RANGE_4_5 = BuildRange(4, 5);

    const TString RANGE_1_1 = BuildRange(1, 1);
    const TString RANGE_5_5 = BuildRange(5, 5);
    const TString RANGE_9_9 = BuildRange(9, 9);

    extern const TString IPV6_FIRST = TAddress::Lowest().AsShortIPv6();
    extern const TString IPV6_LAST  = TAddress::Highest().AsShortIPv6();

    namespace {
        TString GetAddrStr(size_t num) {
            return NIPREG::TAddress::FromIPv4Num(num).AsShortIPv6();
        }
    } // anon-ns

    TString GetFullAddr(const TString& addr) {
        return NIPREG::TAddress::ParseAny(addr).AsShortIPv6();
    }

    TString BuildRange(size_t start, size_t end) {
        if (start > end) {
            throw std::runtime_error("bad range");
        }
        return GetAddrStr(start) + SEP_HYP + GetAddrStr(end) + SEP_TAB;
    }

    TString BuildRange(const TString& start, const TString& end) {
        return start + SEP_HYP + end + SEP_TAB;
    }

    TString BuildFullRangeRow(const TString& start, const TString& end, const TString& data) {
        return BuildRange(start, end) + data;
    }

    TString BuildFullRangeRow(size_t start, size_t end, const TString& data) {
        return BuildRange(start, end) + data;
    }

    TString BuildFullRangeRow(const NIPREG::TAddress& start, const NIPREG::TAddress& end, const TString& data) {
        return BuildFullRangeRow(start.AsIPv6(), end.AsIPv6(), data);
    }

    size_t DetectLinesAmount(const TString& input) {
        size_t lines = 0;
        for (const auto& ch : input) {
            if ('\n' == ch) {
                ++lines;
            }
        }
        return lines;
    }
} // NIpregTest
