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
    TString CheckRanges(const TString& input, bool strict = false) {
        TStringInput inStream(input);
        TStringStream outStream;

        TReader reader(inStream);
        TWriter writer(outStream);

        CheckRangesForMonotonicSequence(reader, writer, strict);
        return outStream.Str();
        }

    bool STRICT_MONOTONIC = true;
} // anon-ns

Y_UNIT_TEST_SUITE(CheckIncrementalTest)
{
    Y_UNIT_TEST(EmptyInput)
    {
        UNIT_ASSERT_STRINGS_EQUAL(CheckRanges(EMPTY), EMPTY);
    }

    Y_UNIT_TEST(SingleFullRange)
    {
        const auto& RANGES = BuildFullRangeRow(IPV6_FIRST, IPV6_LAST, IPREG_DATA_1);
        const auto& output = CheckRanges(RANGES);
        UNIT_ASSERT_STRINGS_EQUAL(output, RANGES);
        UNIT_ASSERT_EQUAL(1, DetectLinesAmount(output));
    }

    Y_UNIT_TEST(SingleBadRange)
    {
        const auto& RANGES = BuildFullRangeRow(IPV6_LAST, IPV6_FIRST, IPREG_DATA_1);
        UNIT_ASSERT_EXCEPTION(CheckRanges(RANGES), std::runtime_error);
    }

    Y_UNIT_TEST(SingleRange)
    {
        const auto& RANGES = BuildFullRangeRow(GetFullAddr("::15"), GetFullAddr("::75"), IPREG_DATA_1);
        const auto& output = CheckRanges(RANGES);
        UNIT_ASSERT_STRINGS_EQUAL(output, RANGES);
        UNIT_ASSERT_EQUAL(1, DetectLinesAmount(output));
    }

    Y_UNIT_TEST(RangesSequence)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, GetFullAddr("1:2:3:4:5:6:7:8"), IPREG_DATA_1) +
            BuildFullRangeRow(GetFullAddr("1:2:3:4:5:6:7:9"), IPV6_LAST,  IPREG_DATA_2);

        const auto& output = CheckRanges(RANGES);
        UNIT_ASSERT_STRINGS_EQUAL(output, RANGES);
        UNIT_ASSERT_EQUAL(2, DetectLinesAmount(output));
    }

    Y_UNIT_TEST(RangesWithHole)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, GetFullAddr("1:2:3:4:5:6:7:8"), IPREG_DATA_1) +
            BuildFullRangeRow(GetFullAddr("1:2:3:4:5:a:b:c"), IPV6_LAST,  IPREG_DATA_2);

        const auto& output = CheckRanges(RANGES);
        UNIT_ASSERT_STRINGS_EQUAL(output, RANGES);
        UNIT_ASSERT_EQUAL(2, DetectLinesAmount(output));
    }

    Y_UNIT_TEST(NetAndSubnet)
    {
        const auto& RANGES =
            BuildFullRangeRow(GetFullAddr("::15"), GetFullAddr("::75"), IPREG_DATA_1) +
            BuildFullRangeRow(GetFullAddr("::15"), GetFullAddr("::25"), IPREG_DATA_2);

        UNIT_ASSERT_EXCEPTION(CheckRanges(RANGES), std::runtime_error);
    }

    Y_UNIT_TEST(MonotonicRanges)
    {
        const auto& RANGES =
            BuildFullRangeRow(1, 3, IPREG_DATA_1) +
            BuildFullRangeRow(5, 6, IPREG_DATA_1) +
            BuildFullRangeRow(7, 8, IPREG_DATA_1) +
            BuildFullRangeRow(8, 9, IPREG_DATA_1) +
            BuildFullRangeRow(9, 9, IPREG_DATA_1);

        const auto& output = CheckRanges(RANGES);
        UNIT_ASSERT_STRINGS_EQUAL(output, RANGES);
        UNIT_ASSERT_EQUAL(5, DetectLinesAmount(output));
    }

    Y_UNIT_TEST(NonMonotonicRanges)
    {
        const auto& RANGES =
            BuildFullRangeRow(1, 4, IPREG_DATA_1) +
            BuildFullRangeRow(3, 6, IPREG_DATA_1) +
            BuildFullRangeRow(7, 9, IPREG_DATA_1);

        UNIT_ASSERT_EXCEPTION(CheckRanges(RANGES), std::runtime_error);
    }

    Y_UNIT_TEST(StrictMonotonicRanges)
    {
        const auto& RANGES =
            BuildFullRangeRow(1, 3, IPREG_DATA_1) +
            BuildFullRangeRow(4, 5, IPREG_DATA_1) +
            BuildFullRangeRow(6, 7, IPREG_DATA_1) +
            BuildFullRangeRow(9, 9, IPREG_DATA_1);

        const auto& output = CheckRanges(RANGES, STRICT_MONOTONIC);
        UNIT_ASSERT_STRINGS_EQUAL(output, RANGES);
        UNIT_ASSERT_EQUAL(4, DetectLinesAmount(output));
    }

    Y_UNIT_TEST(StrictMonotonicRangesFull1)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, IPV6_LAST, IPREG_DATA_1);
        const auto& output = CheckRanges(RANGES, STRICT_MONOTONIC);
        UNIT_ASSERT_STRINGS_EQUAL(output, RANGES);
        UNIT_ASSERT_EQUAL(1, DetectLinesAmount(output));
    }

    Y_UNIT_TEST(StrictMonotonicRangesFull2)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, "::ffff:ffff:ffff", IPREG_DATA_1) +
            BuildFullRangeRow("::1:0:0:0", IPV6_LAST, IPREG_DATA_2);

        const auto& output = CheckRanges(RANGES, STRICT_MONOTONIC);
        UNIT_ASSERT_STRINGS_EQUAL(output, RANGES);
        UNIT_ASSERT_EQUAL(2, DetectLinesAmount(output));
    }

    Y_UNIT_TEST(NonStrictMonotonicRanges)
    {
        const auto& RANGES =
            BuildFullRangeRow(1, 3, IPREG_DATA_1) +
            BuildFullRangeRow(3, 5, IPREG_DATA_1) +
            BuildFullRangeRow(6, 9, IPREG_DATA_1);

        UNIT_ASSERT_EXCEPTION(CheckRanges(RANGES, STRICT_MONOTONIC), std::runtime_error);
    }
} // CheckIncrementalTest
