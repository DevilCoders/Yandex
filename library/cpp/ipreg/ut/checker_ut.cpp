#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/ipreg/checker.h>

using namespace NIPREG;

Y_UNIT_TEST_SUITE(CheckerTest) {
    Y_UNIT_TEST(FlatCheckerInit) {
        UNIT_ASSERT_EQUAL(TFlatChecker().CheckNext(TAddress::ParseIPv6("::0"), TAddress::ParseIPv6("::0")), true);
        UNIT_ASSERT_EQUAL(TFlatChecker().CheckNext(TAddress::ParseIPv6("::0"), TAddress::ParseIPv6("::1")), true);
        UNIT_ASSERT_EQUAL(TFlatChecker().CheckNext(TAddress::ParseIPv6("::1"), TAddress::ParseIPv6("::0")), false);
    }

    Y_UNIT_TEST(FlatChecker) {
        TFlatChecker checker;

        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::0"), TAddress::ParseIPv6("::1")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::2"), TAddress::ParseIPv6("::3")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::5"), TAddress::ParseIPv6("::6")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::6"), TAddress::ParseIPv6("::7")), false);
    }

    Y_UNIT_TEST(IntersectingCheckerInit) {
        UNIT_ASSERT_EQUAL(TIntersectingChecker().CheckNext(TAddress::ParseIPv6("::0"), TAddress::ParseIPv6("::0")), true);
        UNIT_ASSERT_EQUAL(TIntersectingChecker().CheckNext(TAddress::ParseIPv6("::0"), TAddress::ParseIPv6("::1")), true);
        UNIT_ASSERT_EQUAL(TIntersectingChecker().CheckNext(TAddress::ParseIPv6("::1"), TAddress::ParseIPv6("::0")), false);
    }

    Y_UNIT_TEST(IntersectingChecker1) {
        TIntersectingChecker checker;

        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::0"), TAddress::ParseIPv6("::1")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::0"), TAddress::ParseIPv6("::2")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::1"), TAddress::ParseIPv6("::2")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::6"), TAddress::ParseIPv6("::7")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::6"), TAddress::ParseIPv6("::6")), false);
    }

    Y_UNIT_TEST(IntersectingChecker2) {
        TIntersectingChecker checker;

        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::0"), TAddress::ParseIPv6("::1")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::0"), TAddress::ParseIPv6("::2")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::1"), TAddress::ParseIPv6("::2")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::6"), TAddress::ParseIPv6("::7")), true);
        UNIT_ASSERT_EQUAL(checker.CheckNext(TAddress::ParseIPv6("::5"), TAddress::ParseIPv6("::8")), false);
    }
}
