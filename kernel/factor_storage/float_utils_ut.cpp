#include <library/cpp/testing/unittest/registar.h>

#include "float_utils.h"

Y_UNIT_TEST_SUITE(FloatUtilsTest) {
    Y_UNIT_TEST(TestFloatClip) {
        UNIT_ASSERT_EQUAL(SoftClipFloat(0.0f), 0.0f);
        UNIT_ASSERT_EQUAL(SoftClipFloat(1.0f), 1.0f);
        UNIT_ASSERT_EQUAL(SoftClipFloat(0.5f), 0.5f);

        UNIT_ASSERT_EQUAL(SoftClipFloat(1.01f).AbsTolerance(1e-6f), 1.01f);
        UNIT_ASSERT_EQUAL(SoftClipFloat(1.01f).AbsTolerance(1e-2f), 1.0f);
        UNIT_ASSERT_EQUAL(SoftClipFloat(-0.01f).AbsTolerance(1e-6f), -0.01f);
        UNIT_ASSERT_EQUAL(SoftClipFloat(-0.01f).AbsTolerance(1e-2f), 0.0f);

        UNIT_ASSERT_EQUAL(SoftClipFloat(50.0f).Bounds(0, 100.0f), 50.0f);
        UNIT_ASSERT_EQUAL(SoftClipFloat(101.0f).Bounds(0, 100.0f).AbsTolerance(1.0f), 100.f);
    }
};

