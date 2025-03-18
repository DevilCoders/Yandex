#include "regcodes.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NRegion;

Y_UNIT_TEST_SUITE(TRegionsTest) {
    Y_UNIT_TEST(TestRegion) {
        TStringBuf region = "RuSsiA";

        TStringBuf reg(CanonizeRegion(region.data()));
        UNIT_ASSERT_VALUES_EQUAL(reg, TStringBuf("RUS"));

        TStringBuf reg2(CanonizeRegion(643));
        UNIT_ASSERT_VALUES_EQUAL(reg2, TStringBuf("RUS"));

        UNIT_ASSERT_EQUAL(RegionCodeByName("RuS"), 643);
        UNIT_ASSERT_EQUAL(RegionCodeByName("RusSiA"), 643);
        UNIT_ASSERT_EQUAL(RegionCodeByName("Ru"), 643);

        UNIT_ASSERT_VALUES_EQUAL(CanonizeRegion("EN"), TStringBuf("USA"));
        UNIT_ASSERT_VALUES_EQUAL(CanonizeRegion("GER"), TStringBuf("DEU"));
    }
}
