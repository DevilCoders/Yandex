#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/ipreg/reader.h>
#include <library/cpp/ipreg/util_helpers.h>
#include <library/cpp/ipreg/writer.h>

#include "test_helpers.hpp"

using namespace NIPREG;
using namespace NIpregTest;

namespace {
    const auto SORT_DATA = true;

    TString GetMergeResult(
        const TString& input,
        const bool sort = false,
        const TVector<TString>& excludeFieldsList = {},
        const bool countMerge = false,
        const bool joinNestedRanges = false)
    {
        TStringInput inStream(input);
        TStringStream outStream;

        TReader reader(inStream);
        TWriter writer(outStream);

        const MergeTraits mergeTraits{
              excludeFieldsList
            , ""
            , sort
            , countMerge
            , joinNestedRanges
        };

        DoMerging(reader, writer, mergeTraits);
        return outStream.Str();
    }

    void MakeCheck(const TString& input, const TString& wanted, const bool joinNested = false) {
        const bool noSort{};
        const TVector<TString>& excludeFieldsList = {};
        const bool dontCountMerge{};
        const bool joinNestedRanges = joinNested;

        const auto& mergedResult = GetMergeResult(input, noSort, excludeFieldsList, dontCountMerge, joinNestedRanges);
        UNIT_ASSERT_STRINGS_EQUAL(wanted, mergedResult);

        const auto& mergedResultSort = GetMergeResult(input, SORT_DATA, excludeFieldsList, dontCountMerge, joinNestedRanges);
        UNIT_ASSERT_STRINGS_EQUAL(wanted, mergedResultSort);
    }

    template<typename T>
    TString GetShortIpv6(const T& ip) {
        return NIPREG::TAddress::ParseIPv4(ip).AsShortIPv6();
    }

    const auto& NET24_192_88_99 = BuildFullRangeRow(GetShortIpv6("192.88.99.0"), GetShortIpv6("192.88.99.255"), EMPTY_IPREG_DATA);
    const auto& SINGLE_192_88_99_2 =  BuildFullRangeRow(GetShortIpv6("192.88.99.2"), GetShortIpv6("192.88.99.2"), EMPTY_IPREG_DATA);
} // anon-ns

Y_UNIT_TEST_SUITE(UtilMergeTest)
{
    Y_UNIT_TEST(EmptyInput)
    {
        const TString empty;
        MakeCheck(empty, empty);
    }

    Y_UNIT_TEST(NoIntersect)
    {
        const auto& RANGES =
            BuildFullRangeRow(0, 1, IPREG_DATA_1) +
            BuildFullRangeRow(2, 3, IPREG_DATA_2) +
            BuildFullRangeRow(5, 9, IPREG_DATA_2);

        MakeCheck(RANGES, RANGES);
    }

    Y_UNIT_TEST(Intersected)
    {
        const auto& RANGES = // TODO(dieash@) can merge correctly here, with data joining and ranges merging
            BuildFullRangeRow(0, 5, IPREG_DATA_1) +
            BuildFullRangeRow(4, 8, IPREG_DATA_1);

        MakeCheck(RANGES, RANGES);
    }

    Y_UNIT_TEST(BigFirst)
    {
        const auto& WANTED = BuildFullRangeRow(0, 15, IPREG_DATA_1);
        const auto& RANGES = WANTED +
            BuildFullRangeRow(2, 4, IPREG_DATA_1) +
            BuildFullRangeRow(7, 9, IPREG_DATA_1) +
            BuildFullRangeRow(10, 12, IPREG_DATA_1);

        const bool joinNested = true;
        MakeCheck(RANGES, WANTED, joinNested);
    }

    Y_UNIT_TEST(BigFirst_2)
    {
        const auto& WANTED = NET24_192_88_99;
        const auto& RANGES = NET24_192_88_99 + SINGLE_192_88_99_2;

        const bool joinNested = true;
        MakeCheck(RANGES, WANTED, joinNested);

        MakeCheck(RANGES, RANGES);  // NB, no join-nested
    }

    Y_UNIT_TEST(SmallFirst)
    {
        const auto& WANTED = BuildFullRangeRow(0, 15, IPREG_DATA_1);
        const auto& RANGES = BuildFullRangeRow(0, 12, IPREG_DATA_1) + WANTED;
        MakeCheck(RANGES, WANTED);
    }

    Y_UNIT_TEST(SmallFirst_2)
    {
        const auto& WANTED = NET24_192_88_99;
        const auto& RANGES = SINGLE_192_88_99_2 + NET24_192_88_99;

        const bool joinNested = true;
        MakeCheck(RANGES, WANTED, joinNested);

        MakeCheck(RANGES, WANTED); // NB: no matter, without join-nested
    }

    Y_UNIT_TEST(Sequence)
    {
        const auto& RANGES =
            BuildFullRangeRow(0, 1, IPREG_DATA_1) +
            BuildFullRangeRow(2, 4, IPREG_DATA_1) +
            BuildFullRangeRow(5, 9, IPREG_DATA_1);
        const auto& WANTED = BuildFullRangeRow(0, 9, IPREG_DATA_1);

        MakeCheck(RANGES, WANTED);
    }

    Y_UNIT_TEST(SequenceUnsorted)
    {
        const auto& RANGES =
            BuildFullRangeRow(0, 1, IPREG_UNSORT_REAL_DATA_1) +
            BuildFullRangeRow(2, 4, IPREG_UNSORT_REAL_DATA_2) +
            BuildFullRangeRow(5, 9, IPREG_UNSORT_REAL_DATA_3);
        const auto& WANTED = BuildFullRangeRow(0, 9, IPREG_SORTED_REAL_DATA);

        const auto& result1 = GetMergeResult(RANGES);
        UNIT_ASSERT(result1 != WANTED);

        const auto& result2 = GetMergeResult(RANGES, SORT_DATA);
        UNIT_ASSERT_STRINGS_EQUAL(result2, WANTED);
    }

    Y_UNIT_TEST(MergeWithIgnoring)
    {
        const TString REAL_DATA_1 = "{\"reliability\":2,\"region_id\":146}" + ROW_END;
        const TString REAL_DATA_2 = "{\"is_yandex_staff\":1,\"region_id\":146,\"reliability\":5}" + ROW_END;
        const TString REAL_DATA_3 = "{\"reliability\":7,\"region_id\":146}" + ROW_END;

        const TString REAL_DATA_MERGED1 = "{\"_mrg_qty_\":2,\"region_id\":146,\"reliability\":2}" + ROW_END;
        const TString REAL_DATA_MERGED2 = "{\"_mrg_qty_\":2,\"region_id\":146,\"reliability\":7}" + ROW_END;

        const auto& RANGES =
            BuildFullRangeRow(0, 1, REAL_DATA_1) +
            BuildFullRangeRow(2, 3, REAL_DATA_3) +
            BuildFullRangeRow(4, 5, REAL_DATA_2) +
            BuildFullRangeRow(6, 7, REAL_DATA_3) +
            BuildFullRangeRow(8, 9, REAL_DATA_1);

        const auto& WANTED =
            BuildFullRangeRow(0, 3, REAL_DATA_MERGED1) +
            BuildFullRangeRow(4, 5, REAL_DATA_2) +
            BuildFullRangeRow(6, 9, REAL_DATA_MERGED2);

        const auto sortKeys = true;
        const auto countMerges = true;
        const TVector<TString> excludes = { "reliability" };

        const auto& mergedResult = GetMergeResult(RANGES, sortKeys, excludes, countMerges);
        UNIT_ASSERT_STRINGS_EQUAL(WANTED, mergedResult);
    }

    Y_UNIT_TEST(MergeFullIpSpace)
    {
        const auto IP_SPACE_START = TAddress::Lowest();
        const auto IPV4_SPACE_START = TAddress::ParseIPv4("0.0.0.0");
        const auto IPV4_SPACE_END =  TAddress::ParseIPv4("255.255.255.255");
        const auto IP_SPACE_END = TAddress::Highest();

        const auto& RANGES =
            BuildFullRangeRow(IP_SPACE_START, IPV4_SPACE_START.Prev(), EMPTY_IPREG_DATA) +
            BuildFullRangeRow(IPV4_SPACE_START, IPV4_SPACE_END, EMPTY_IPREG_DATA) +
            BuildFullRangeRow(IPV4_SPACE_END.Next(), IP_SPACE_END, EMPTY_IPREG_DATA);

        const auto& WANTED =
            BuildFullRangeRow(IP_SPACE_START.AsShortIP(), IP_SPACE_END.AsShortIP(), EMPTY_IPREG_DATA);

        MakeCheck(RANGES, WANTED);
    }
} // UtilMergeTest
