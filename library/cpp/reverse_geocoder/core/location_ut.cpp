#include <library/cpp/testing/unittest/registar.h>

#include "location.h"

using namespace NReverseGeocoder;

Y_UNIT_TEST_SUITE(TLocation) {
    Y_UNIT_TEST(DefaultConstructor) {
        TLocation location;
        UNIT_ASSERT_DOUBLES_EQUAL(0.0, location.Lon, 1e-12);
        UNIT_ASSERT_DOUBLES_EQUAL(0.0, location.Lat, 1e-12);
    }

    Y_UNIT_TEST(Constructor) {
        TLocation location(37.562980, 55.771113);
        UNIT_ASSERT_DOUBLES_EQUAL(37.562980, location.Lon, 1e-12);
        UNIT_ASSERT_DOUBLES_EQUAL(55.771113, location.Lat, 1e-12);
    }
}
