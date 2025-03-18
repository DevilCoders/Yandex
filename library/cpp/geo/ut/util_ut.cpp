#include <library/cpp/geo/util.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NGeo;

Y_UNIT_TEST_SUITE(TGeoUtilTest) {
    Y_UNIT_TEST(TestPointFromString) {
        UNIT_ASSERT_EQUAL(PairFromString("27.56,53.90"), (std::pair<double, double>(27.56, 53.90)));
        UNIT_ASSERT_EQUAL(PairFromString("27.56 53.90", " "), (std::pair<double, double>(27.56, 53.90)));
        UNIT_ASSERT_EQUAL(PairFromString("27.56@@53.90", "@@"), (std::pair<double, double>(27.56, 53.90)));
        UNIT_ASSERT_EXCEPTION(PairFromString("27.56@@53.90", "@"), TBadCastException);
        UNIT_ASSERT_EXCEPTION(PairFromString(""), TBadCastException);
    }

    Y_UNIT_TEST(TestTryPointFromString) {
        std::pair<double, double> point;

        UNIT_ASSERT(TryPairFromString(point, "27.56,53.90"));
        UNIT_ASSERT_EQUAL(point, (std::pair<double, double>(27.56, 53.90)));

        UNIT_ASSERT(TryPairFromString(point, "27.56 53.90", " "));
        UNIT_ASSERT_EQUAL(point, (std::pair<double, double>(27.56, 53.90)));

        UNIT_ASSERT(TryPairFromString(point, "27.56@@53.90", "@@"));
        UNIT_ASSERT_EQUAL(point, (std::pair<double, double>(27.56, 53.90)));

        UNIT_ASSERT(!TryPairFromString(point, "27.56@@53.90", "@"));
        UNIT_ASSERT(!TryPairFromString(point, ""));
    }
}
