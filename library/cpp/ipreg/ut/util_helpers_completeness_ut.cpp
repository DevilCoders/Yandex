#include <library/cpp/testing/unittest/registar.h>

#include "test_helpers.hpp"

#include <library/cpp/ipreg/reader.h>
#include <library/cpp/ipreg/util_helpers.h>
#include <library/cpp/ipreg/writer.h>

#include <util/generic/vector.h>
#include <util/string/split.h>

using namespace NIPREG;
using namespace NIpregTest;

namespace {
    TString CheckAddrSpace(const TString& input) {
        TStringInput inStream(input);
        TStringStream outStream;

        TReader reader(inStream);
        TWriter writer(outStream);

        CheckAddressSpaceForCompleteness(reader, writer);
        return outStream.Str();
    }
} // anon-ns

Y_UNIT_TEST_SUITE(AddrSpaceTest)
{
    Y_UNIT_TEST(EmptyInput)
    {
        UNIT_ASSERT_EXCEPTION(CheckAddrSpace(EMPTY), std::runtime_error);
    }

    Y_UNIT_TEST(SingleFullRange)
    {
        const auto& RANGES = BuildFullRangeRow(IPV6_FIRST, IPV6_LAST, IPREG_DATA_1);
        const auto& output = CheckAddrSpace(RANGES);
        UNIT_ASSERT_EQUAL(1, DetectLinesAmount(output));
    }

    Y_UNIT_TEST(SingleRange)
    {
        const auto& RANGES = BuildFullRangeRow(GetFullAddr("::15"), GetFullAddr("::75"), IPREG_DATA_1);
        UNIT_ASSERT_EXCEPTION(CheckAddrSpace(RANGES), std::runtime_error);
    }

    Y_UNIT_TEST(BadEnd)
    {
        const auto& RANGES = BuildFullRangeRow(IPV6_FIRST, GetFullAddr("::75"), IPREG_DATA_2);
        UNIT_ASSERT_EXCEPTION(CheckAddrSpace(RANGES), std::runtime_error);
    }

    Y_UNIT_TEST(TwoRanges)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, GetFullAddr("1:2:3:4:5:6:7:8"), IPREG_DATA_1) +
            BuildFullRangeRow(GetFullAddr("1:2:3:4:5:6:7:9"), IPV6_LAST,  IPREG_DATA_2);

        const auto& output = CheckAddrSpace(RANGES);
        UNIT_ASSERT_EQUAL(2, DetectLinesAmount(output));
    }

    Y_UNIT_TEST(Ranges2_1)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, IPV6_LAST, IPREG_DATA_1) +
            BuildFullRangeRow(IPV6_LAST, IPV6_LAST, IPREG_DATA_1);
        UNIT_ASSERT_EXCEPTION(CheckAddrSpace(RANGES), std::runtime_error);
    }

    Y_UNIT_TEST(Ranges2_2)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, IPV6_FIRST, IPREG_DATA_1) +
            BuildFullRangeRow(IPV6_FIRST, IPV6_LAST, IPREG_DATA_1);
        UNIT_ASSERT_EXCEPTION(CheckAddrSpace(RANGES), std::runtime_error);
    }

    Y_UNIT_TEST(Ranges2_3)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, "::ffff:0:1", IPREG_DATA_1) +
            BuildFullRangeRow("::ffff:0:1", IPV6_LAST, IPREG_DATA_1);
        UNIT_ASSERT_EXCEPTION(CheckAddrSpace(RANGES), std::runtime_error);
    }

    Y_UNIT_TEST(RangesWithHole)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, GetFullAddr("1:2:3:4:5:6:7:8"), IPREG_DATA_1) +
            BuildFullRangeRow(GetFullAddr("1:2:3:4:5:6::"), IPV6_LAST,  IPREG_DATA_2);

        UNIT_ASSERT_EXCEPTION(CheckAddrSpace(RANGES), std::runtime_error);
    }

    Y_UNIT_TEST(SomeRanges)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, "::ffff:0:1", IPREG_DATA_1) +
            BuildFullRangeRow(2, 4, IPREG_UNSORT_REAL_DATA_1) +
            BuildFullRangeRow(5, 6, IPREG_DATA_2) +
            BuildFullRangeRow(7, 9, IPREG_UNSORT_REAL_DATA_3) +
            BuildFullRangeRow("::ffff:0:a", IPV6_LAST, IPREG_DATA_3);

        const auto& output = CheckAddrSpace(RANGES);
        UNIT_ASSERT_EQUAL(5, DetectLinesAmount(output));
    }
} // UtilStubsTest
