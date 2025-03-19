#include <kernel/common_server/util/math/math.h>

#include <library/cpp/testing/unittest/registar.h>

#include <limits>

Y_UNIT_TEST_SUITE(Math) {
    Y_UNIT_TEST(TestSuccess1) {
        const ui32 x = 2;
        const i32 y = 2;
        UNIT_ASSERT_EQUAL(NMath::Add(x, y), 4);
    }

    Y_UNIT_TEST(TestSuccess2) {
        const ui32 x = 2;
        const i32 y = -2;
        UNIT_ASSERT_EQUAL(NMath::Add(x, y), 0);
    }

    Y_UNIT_TEST(TestException1) {
        const ui32 x = 2;
        const i32 y = -3;
        UNIT_ASSERT_EXCEPTION_CONTAINS(NMath::Add(x, y), yexception, "Subtrahend is too big");
    }

    Y_UNIT_TEST(TestException2) {
        const ui32 x = std::numeric_limits<ui32>::max();
        const i32 y = 1;
        UNIT_ASSERT_EXCEPTION_CONTAINS(NMath::Add(x, y), yexception, "Terms are too big");
    }
}
