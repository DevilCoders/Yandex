#include <library/cpp/testing/unittest/registar.h>

#include "common.h"

using namespace NReverseGeocoder;

Y_UNIT_TEST_SUITE(Common) {
    Y_UNIT_TEST(CheckSizes) {
        UNIT_ASSERT_EQUAL(4, sizeof(TCoordinate));
        UNIT_ASSERT_EQUAL(8, sizeof(TGeoId));
        UNIT_ASSERT_EQUAL(4, sizeof(TNumber));
        UNIT_ASSERT_EQUAL(4, sizeof(TRef));
        UNIT_ASSERT_EQUAL(8, sizeof(TSquare));
        UNIT_ASSERT_EQUAL(8, sizeof(TVersion));
    }

    Y_UNIT_TEST(Convertion) {
        UNIT_ASSERT_EQUAL(1000000, ToCoordinate(1.0));
        UNIT_ASSERT_EQUAL(-7777221, ToCoordinate(-7.777221));

        UNIT_ASSERT_DOUBLES_EQUAL(1.0, ToDouble(1000000), 1e-12);
        UNIT_ASSERT_DOUBLES_EQUAL(-7.777221, ToDouble(-7777221), 1e-12);
    }
}
