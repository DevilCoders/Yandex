#include <library/cpp/vec4/math.h>

#include <library/cpp/testing/unittest/registar.h>

#include <cmath>

Y_UNIT_TEST_SUITE(Vec4MathTest){
    Y_UNIT_TEST(TestSingleFastInvSqrt){
        float val = 1e-30;
while (val < 1e30) {
    val = 1.47821 * val * (1 + 0.141461 * cos(val + 1 / val));
    const double trueInvSqrt = 1.0 / sqrt(val);
    const double fast = FastInvSqrt(val);
    UNIT_ASSERT_DOUBLES_EQUAL(trueInvSqrt, fast, trueInvSqrt / 2048.0);
}
}
}
;
