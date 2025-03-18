#include "prime.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(PrimeTest) {
    Y_UNIT_TEST(TestNextPrime) {
        UNIT_ASSERT_EQUAL(NextPrime(12), 13);
        UNIT_ASSERT_EQUAL(NextPrime(13), 17);
        UNIT_ASSERT_EQUAL(NextPrime(62354), 62383);
    }
    Y_UNIT_TEST(TestIsPrime) {
        UNIT_ASSERT(IsPrime(37));
        UNIT_ASSERT(!IsPrime(32));
        UNIT_ASSERT(IsPrime(62383));
        UNIT_ASSERT(!IsPrime(62381));
    }
}
