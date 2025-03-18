#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/ipreg/reader.h>
#include <library/cpp/ipreg/util_helpers.h>
#include <library/cpp/ipreg/writer.h>

#include "test_helpers.hpp"

using namespace NIPREG;
using namespace NIpregTest;

namespace {
    TString AddStubs(const TString& input) {
        TStringInput inStream(input);
        TStringStream outStream;

        TReader reader(inStream);
        TWriter writer(outStream);

        AddStubRanges(reader, writer);
        return outStream.Str();
    }

    TString GetStubData() {
        return NIPREG::STUB_DATA + ROW_END;
    }
} // anon-ns

Y_UNIT_TEST_SUITE(UtilStubsTest)
{
    Y_UNIT_TEST(EmptyInput)
    {
        const auto& RANGES = BuildFullRangeRow(IPV6_FIRST, IPV6_LAST, GetStubData());

        const auto& result = AddStubs(EMPTY);
        UNIT_ASSERT_STRINGS_EQUAL(RANGES, result);
    }

    Y_UNIT_TEST(WithoutStubsRange)
    {
        const auto& RANGES = BuildFullRangeRow(IPV6_FIRST, IPV6_LAST, IPREG_DATA_1);

        const auto& result = AddStubs(RANGES);
        UNIT_ASSERT_STRINGS_EQUAL(RANGES, result);
    }

    Y_UNIT_TEST(WithoutStubsRanges)
    {
        const auto& RANGES =
            BuildFullRangeRow(IPV6_FIRST, GetFullAddr("1:2:3:4:5:6:7:8"), IPREG_DATA_1) +
            BuildFullRangeRow(GetFullAddr("1:2:3:4:5:6:7:9"), IPV6_LAST,  IPREG_DATA_2);

        const auto& result = AddStubs(RANGES);
        UNIT_ASSERT_STRINGS_EQUAL(RANGES, result);
    }

    Y_UNIT_TEST(SingleRangeInside)
    {
        const auto& RANGE = BuildFullRangeRow(GetFullAddr("::15"), GetFullAddr("::75"), IPREG_DATA_1);
        const auto& WANTED_STUB_RANGE_STUB =
            BuildFullRangeRow(IPV6_FIRST, GetFullAddr("::14"), GetStubData()) +
            RANGE +
            BuildFullRangeRow(GetFullAddr("::76"), IPV6_LAST, GetStubData());
        const auto& result = AddStubs(RANGE);
        UNIT_ASSERT_STRINGS_EQUAL(WANTED_STUB_RANGE_STUB, result);
    }

    Y_UNIT_TEST(RangesAndHoles)
    {
        const auto& RANGE_1 = BuildFullRangeRow(GetFullAddr("::ffff:1.2.3.4"), GetFullAddr("::ffff:123.234.34.45"), IPREG_DATA_1);
        const auto& RANGE_2 = BuildFullRangeRow(GetFullAddr("1:2:3:4:5:6:7:8"), GetFullAddr("12:23:34:45:56:67:78:89"), IPREG_DATA_2);

        const auto& RANGES = RANGE_1 + RANGE_2;

        const auto& WANTED_RANGES_WITH_STUBS =
            BuildFullRangeRow(IPV6_FIRST, GetFullAddr("::ffff:1.2.3.3"), GetStubData()) +
            RANGE_1 +
            BuildFullRangeRow(GetFullAddr("::ffff:123.234.34.46"), GetFullAddr("1:2:3:4:5:6:7:7"), GetStubData()) +
            RANGE_2 +
            BuildFullRangeRow(GetFullAddr("12:23:34:45:56:67:78:8a"), IPV6_LAST, GetStubData());

        const auto& result = AddStubs(RANGES);
        UNIT_ASSERT_STRINGS_EQUAL(WANTED_RANGES_WITH_STUBS, result);
    }

    Y_UNIT_TEST(ExceptionWithMixedRanges)
    {
        const auto& RANGES =
            BuildFullRangeRow(15, 35, IPREG_DATA_1) +
            BuildFullRangeRow(25, 45, IPREG_DATA_2);

        // TODO(dieash@)
        // NB: in Perl we got very STRANGE ranges.
        //     I think it's ugly bug so fix it.
        //
        //   ::ffff:0000:000f-::ffff:0000:0023 ipreg_data_1
        //   ::ffff:0000:0024-::ffff:0000:0018 {is_placeholder}  # <== NOTA BENE
        //   ::ffff:0000:0019-::ffff:0000:002d ipreg_data_2

        UNIT_ASSERT_EXCEPTION(AddStubs(RANGES), std::runtime_error);
    }
} // UtilStubsTest
