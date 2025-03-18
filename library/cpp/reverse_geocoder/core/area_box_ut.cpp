#include <library/cpp/testing/unittest/registar.h>

#include "area_box.h"

using namespace NReverseGeocoder;

Y_UNIT_TEST_SUITE(TAreaBox) {
    Y_UNIT_TEST(TestAreaBoxCount) {
        UNIT_ASSERT_EQUAL(static_cast<int>(6480000u), static_cast<int>(NAreaBox::Number));
    }
}
