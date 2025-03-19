#include <kernel/geo/utils.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TQualityUtilsGeoUtilsTest) {
    Y_UNIT_TEST(IsRelevfmlCountryRegionTest) {
        TGeoRegion nonCountries[] = {42, 100500, 100, 1, 2, 3, 4, 99};
        TGeoRegion countries[] = {0, 225, 187, 159, 149, 983, 120, 117, 206, 179, 208, 10077, 96, 84, 169, 171, 181, 124, 95, 102, 116, 167, 115, 994};

        for (size_t i = 0; i < Y_ARRAY_SIZE(nonCountries); i++) {
            UNIT_ASSERT_EQUAL(IsRelevfmlCountryRegion(nonCountries[i]), 0);
        }

        for (size_t i = 0; i < Y_ARRAY_SIZE(countries); i++) {
            UNIT_ASSERT_EQUAL(IsRelevfmlCountryRegion(countries[i]), 1);
        }
    }

    Y_UNIT_TEST(TestRrrExplicitBool) {
        TRelevRegionResolver rrr;
        UNIT_ASSERT(!rrr);
    }

    Y_UNIT_TEST(DebugDB) {
        TRegionsDB testDB("debug://225:1:213");
        UNIT_ASSERT_EQUAL(testDB.GetParent(213), 1);
        UNIT_ASSERT_EQUAL(testDB.GetParent(1), 225);
        UNIT_ASSERT_EQUAL(testDB.GetParent(225), -1);
    }
}

