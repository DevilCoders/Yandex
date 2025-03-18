#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/ipreg/reader.h>
#include <library/cpp/ipreg/util_helpers.h>
#include <library/cpp/ipreg/writer.h>

#include "test_helpers.hpp"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>

using namespace NIPREG;
using namespace NIpregTest;

namespace {
    const auto SORT_DATA = true;

    TString GetPatchResult(const TString& input, const TString& patch, bool sortData = false) {
        TStringInput inputStream(input);
        TStringInput patchStream(patch);
        TStringStream outStream;

        TReader inputReader(inputStream);
        TReader patchReader(patchStream);
        TWriter writer(outStream);

        DoPatching(inputReader, patchReader, writer, sortData);
        return outStream.Str();
    }

    void Check(const TString& ipreg, const TString& patch, const TString& wanted) {
        const auto& patched = GetPatchResult(ipreg, patch);
        UNIT_ASSERT_STRINGS_EQUAL(wanted, patched);
    }

} // anon-ns

Y_UNIT_TEST_SUITE(UtilPatchTest) {
    Y_UNIT_TEST(Empty) {
        Check(EMPTY, EMPTY, EMPTY);
    }

    Y_UNIT_TEST(NoPatch) {
        Check(SOME_RANGES_DATA, EMPTY, SOME_RANGES_DATA);
    }

    Y_UNIT_TEST(NoInput) {
        Check(EMPTY, SOME_RANGES_DATA, SOME_RANGES_DATA);
    }

    Y_UNIT_TEST(BaseBeforePatch) {
        const auto& IPREG = RANGE_1_5 + IPREG_DATA_1;
        const auto& PATCH = RANGE_6_9 + PATCH_DATA_1;

        Check(IPREG, PATCH, IPREG + PATCH);
    }

    Y_UNIT_TEST(BaseAfterPatch) {
        const auto& IPREG = RANGE_6_9 + IPREG_DATA_1;
        const auto& PATCH = RANGE_1_5 + PATCH_DATA_1;

        Check(IPREG, PATCH, PATCH + IPREG);
    }

    Y_UNIT_TEST(FullIntersect) {
        const auto& IPREG = RANGE_1_5 + IPREG_DATA_1;
        const auto& PATCH = RANGE_1_5 + PATCH_DATA_1;
        const auto& WANTED_RANGES = BuildFullRangeRow(1, 5, IPREG_DATA_1_PATCH_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(FullIntersect2) {
        const auto& RANGE = BuildRange("4000::", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
        const auto& IPREG = RANGE + IPREG_DATA_1;
        const auto& PATCH = RANGE + PATCH_DATA_1;
        const auto& WANTED_RANGES = RANGE + IPREG_DATA_1_PATCH_1;

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(BaseWithinPatch) {
        const auto& IPREG = RANGE_1_9 + IPREG_DATA_1;
        const auto& PATCH = RANGE_4_5 + PATCH_DATA_1;

        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 3, IPREG_DATA_1) +
            BuildFullRangeRow(4, 5, IPREG_DATA_1_PATCH_1) +
            BuildFullRangeRow(6, 9, IPREG_DATA_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(BaseEndInPatch) {
        const auto& IPREG = RANGE_1_7 + IPREG_DATA_1;
        const auto& PATCH = RANGE_5_9 + PATCH_DATA_1;

        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 4, IPREG_DATA_1) +
            BuildFullRangeRow(5, 7, IPREG_DATA_1_PATCH_1) +
            BuildFullRangeRow(8, 9, PATCH_DATA_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(BaseBeginInPatch) {
        const auto& IPREG = RANGE_5_9 + IPREG_DATA_1;
        const auto& PATCH = RANGE_1_7 + PATCH_DATA_1;

        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 4, PATCH_DATA_1) +
            BuildFullRangeRow(5, 7, IPREG_DATA_1_PATCH_1) +
            BuildFullRangeRow(8, 9, IPREG_DATA_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(BaseInPatch) {
        const auto& IPREG = RANGE_4_5 + IPREG_DATA_1;
        const auto& PATCH = RANGE_1_9 + PATCH_DATA_1;

        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 3, PATCH_DATA_1) +
            BuildFullRangeRow(4, 5, IPREG_DATA_1_PATCH_1) +
            BuildFullRangeRow(6, 9, PATCH_DATA_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(Misc_LikePerlImpl) {
        const auto RANGE_END = (2 << 23) - 1;
        const auto& IPREG =
            BuildFullRangeRow(0, 0, IPREG_DATA_1) +
            BuildFullRangeRow(1, RANGE_END, IPREG_DATA_1);
        const auto& PATCH = BuildFullRangeRow(0, RANGE_END, PATCH_DATA_2);

        const auto& WANTED_RANGES =
            BuildFullRangeRow(0, 0, IPREG_DATA_1_PATCH_2) +
            BuildFullRangeRow(1, RANGE_END, IPREG_DATA_1_PATCH_2);

        // TODO(dieash@)
        // NB: in Perl we got _2_ patched ranges.
        //
        // patch data: {"rel":0,"reg":4}
        // usual data: {"rel":1,"reg":2}
        //
        // patch result:
        //   ::ffff:0000:0000-::ffff:0000:0000 {"reg":4,"rel":0}
        //   ::ffff:0000:0001-::ffff:00ff:ffff {"rel":0,"reg":4}

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(SameSingleAddr) {
        const auto& IPREG = RANGE_5_5 + IPREG_DATA_1;
        const auto& PATCH = RANGE_5_5 + PATCH_DATA_1;

        const auto& WANTED_RANGES = RANGE_5_5 + IPREG_DATA_1_PATCH_1;

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(SingleAddrPatchBegin) {
        const auto& IPREG = RANGE_1_9 + IPREG_DATA_1;
        const auto& PATCH = RANGE_1_1 + PATCH_DATA_1;

        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 1, IPREG_DATA_1_PATCH_1) +
            BuildFullRangeRow(2, 9, IPREG_DATA_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(RangePatchInBegin) {
        const auto& IPREG = RANGE_1_9 + IPREG_DATA_1;
        const auto& PATCH = RANGE_1_5 + PATCH_DATA_1;
        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 5, IPREG_DATA_1_PATCH_1) +
            BuildFullRangeRow(6, 9, IPREG_DATA_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(RangePatchInBeginRealSample) {
        const auto& IPREG = "0000:0000:0000:0000:0000:ffff:0101:8000-0000:0000:0000:0000:0000:ffff:0101:ffff\t{\"route\":\"original\"}\n";
        const auto& PATCH = "0000:0000:0000:0000:0000:ffff:0101:8000-0000:0000:0000:0000:0000:ffff:0101:87ff\t{\"route\":\"corrected\"}\n";
        const auto& WANTED_RANGES =
            "0000:0000:0000:0000:0000:ffff:0101:8000-0000:0000:0000:0000:0000:ffff:0101:87ff\t{\"route\":\"corrected\"}\n"
            "0000:0000:0000:0000:0000:ffff:0101:8800-0000:0000:0000:0000:0000:ffff:0101:ffff\t{\"route\":\"original\"}\n";

        SetIpFullOutFormat();
        Check(IPREG, PATCH, WANTED_RANGES);
        SetIpShortOutFormat();
    }

    Y_UNIT_TEST(SingleAddrPatchEnd) {
        const auto& IPREG = RANGE_1_9 + IPREG_DATA_1;
        const auto& PATCH = RANGE_9_9 + PATCH_DATA_1;

        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 8, IPREG_DATA_1) +
            BuildFullRangeRow(9, 9, IPREG_DATA_1_PATCH_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(SingleAddrPatchInside) {
        const auto& IPREG = RANGE_1_9 + IPREG_DATA_1;
        const auto& PATCH = RANGE_5_5 + PATCH_DATA_1;

        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 4, IPREG_DATA_1) +
            BuildFullRangeRow(5, 5, IPREG_DATA_1_PATCH_1) +
            BuildFullRangeRow(6, 9, IPREG_DATA_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(SingleAddrBegin) {
        const auto& IPREG = RANGE_1_1 + IPREG_DATA_1;
        const auto& PATCH = RANGE_1_9 + PATCH_DATA_1;

        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 1, IPREG_DATA_1_PATCH_1) +
            BuildFullRangeRow(2, 9, PATCH_DATA_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(SingleAddrEnd) {
        const auto& IPREG = RANGE_9_9 + IPREG_DATA_1;
        const auto& PATCH = RANGE_1_9 + PATCH_DATA_1;

        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 8, PATCH_DATA_1) +
            BuildFullRangeRow(9, 9, IPREG_DATA_1_PATCH_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(SingleAddrInsidePatch) {
        const auto& IPREG = RANGE_5_5 + IPREG_DATA_1;
        const auto& PATCH = RANGE_1_9 + PATCH_DATA_1;

        const auto& WANTED_RANGES =
            BuildFullRangeRow(1, 4, PATCH_DATA_1) +
            BuildFullRangeRow(5, 5, IPREG_DATA_1_PATCH_1) +
            BuildFullRangeRow(6, 9, PATCH_DATA_1);

        Check(IPREG, PATCH, WANTED_RANGES);
    }

    Y_UNIT_TEST(ReliabilityPatch) {
        const TString& IPREG_JSON = "{\"reliability\":5,\"region_id\":2}" + ROW_END;
        const auto& IPREG = BuildFullRangeRow("::ffff:52d:c000", "::ffff:52d:c5ab", IPREG_JSON);
        const auto& PATCH_RANGE = BuildRange("::ffff:52d:c200", "::ffff:52d:c3ff");

        const TString& PATCH_JSON_NO_REL = "{\"is_yandex_staff\":1,\"region_id\":213}" + ROW_END;
        const TString& PATCH_JSON_REL_0  = "{\"region_id\":213,\"is_yandex_staff\":1,\"reliability\":0}" + ROW_END;

        const auto& RANGE_BEFORE_PATCH = BuildFullRangeRow("::ffff:52d:c000", "::ffff:52d:c1ff", IPREG_JSON);
        const auto& RANGE_AFTER_PATCH  = BuildFullRangeRow("::ffff:52d:c400", "::ffff:52d:c5ab", IPREG_JSON);

        { // NO realibility
            const auto& IPREG_PATCH_DATA = "{\"region_id\":213,\"is_yandex_staff\":1,\"reliability\":5}" + ROW_END;

            const auto& WANTED_RANGES =
                RANGE_BEFORE_PATCH +
                PATCH_RANGE + IPREG_PATCH_DATA +
                RANGE_AFTER_PATCH;

            Check(IPREG, PATCH_RANGE + SEP_TAB + PATCH_JSON_NO_REL, WANTED_RANGES);
        }

        { // EXISTS realibility
            const auto& WANTED_RANGES =
                RANGE_BEFORE_PATCH +
                PATCH_RANGE + PATCH_JSON_REL_0 +
                RANGE_AFTER_PATCH;

            Check(IPREG, PATCH_RANGE + SEP_TAB + PATCH_JSON_REL_0, WANTED_RANGES);
        }
    }

    Y_UNIT_TEST(InputUnsortNoPatch) {
        const auto& RANGES =
            BuildFullRangeRow(0, 1, IPREG_UNSORT_REAL_DATA_1) +
            BuildFullRangeRow(2, 4, IPREG_UNSORT_REAL_DATA_2) +
            BuildFullRangeRow(5, 9, IPREG_UNSORT_REAL_DATA_3);

        const auto& patched = GetPatchResult(RANGES, EMPTY);
        UNIT_ASSERT_STRINGS_EQUAL(RANGES, patched);

        const auto& patchSort = GetPatchResult(RANGES, EMPTY, SORT_DATA);
        UNIT_ASSERT_STRINGS_EQUAL(RANGES, patchSort);
    }

    Y_UNIT_TEST(InputUnsortPatchAndSort) {
        const auto& RANGES =
            BuildFullRangeRow(0, 1, IPREG_UNSORT_REAL_DATA_1) +
            BuildFullRangeRow(2, 4, IPREG_UNSORT_REAL_DATA_2) +
            BuildFullRangeRow(5, 9, IPREG_UNSORT_REAL_DATA_3);

        const auto& patched = GetPatchResult(RANGES, EMPTY);
        UNIT_ASSERT_STRINGS_EQUAL(RANGES, patched);

        const TString IPREG_PATCH_DATA = "{\"reliability\":20.2,\"is_yandex_staff\":777}" + ROW_END;
        const auto& PATCH = BuildFullRangeRow(2, 4, IPREG_PATCH_DATA);

        const TString IPREG_SORTED_PATCHED_DATA_2 = "{\"is_placeholder\":0,\"is_yandex_staff\":777,\"region_id\":225,\"reliability\":20.2}" + ROW_END;

        const auto& WANTED =
            BuildFullRangeRow(0, 1, IPREG_UNSORT_REAL_DATA_1) +
            BuildFullRangeRow(2, 4, IPREG_SORTED_PATCHED_DATA_2) +
            BuildFullRangeRow(5, 9, IPREG_UNSORT_REAL_DATA_3);

        const auto& patchSort = GetPatchResult(RANGES, PATCH, SORT_DATA);
        UNIT_ASSERT_STRINGS_EQUAL(WANTED, patchSort);
    }
} // UtilPatchTest
