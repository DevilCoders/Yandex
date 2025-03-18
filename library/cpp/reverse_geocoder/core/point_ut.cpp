#include <library/cpp/testing/unittest/registar.h>

#include "point.h"

using namespace NReverseGeocoder;

Y_UNIT_TEST_SUITE(TPoint) {
    Y_UNIT_TEST(DefaultConstructor) {
        TPoint p;
        UNIT_ASSERT_EQUAL(0, p.X);
        UNIT_ASSERT_EQUAL(0, p.Y);
    }

    Y_UNIT_TEST(Constructor) {
        TPoint p(22, -33);
        UNIT_ASSERT_EQUAL(22, p.X);
        UNIT_ASSERT_EQUAL(-33, p.Y);
    }

    Y_UNIT_TEST(LocationConstructor) {
        TLocation location(37.5629801, 55.771113);
        TPoint point(location);

        UNIT_ASSERT_EQUAL(37562980, point.X);
        UNIT_ASSERT_EQUAL(55771113, point.Y);
    }

    Y_UNIT_TEST(OperatorMinus) {
        TPoint a(22, 33);
        TPoint b(17, -33);
        TPoint c = a - b;

        UNIT_ASSERT_EQUAL((22 - 17), c.X);
        UNIT_ASSERT_EQUAL((33 + 33), c.Y);
    }

    Y_UNIT_TEST(OperatorEquals) {
        TPoint a(22, 33);
        TPoint b(22, 33);
        TPoint c(23, 33);
        TPoint d(22, 32);
        TPoint e(100, 77);

        UNIT_ASSERT(a == b);
        UNIT_ASSERT(!(a == c));
        UNIT_ASSERT(!(a == d));
        UNIT_ASSERT(!(a == e));
    }

    Y_UNIT_TEST(OperatorNotEquals) {
        TPoint a(22, 33);
        TPoint b(22, 33);
        TPoint c(23, 33);
        TPoint d(22, 32);
        TPoint e(100, 77);

        UNIT_ASSERT(!(a != b));
        UNIT_ASSERT(a != c);
        UNIT_ASSERT(a != d);
        UNIT_ASSERT(a != e);
    }

    Y_UNIT_TEST(OperatorLess) {
        TPoint a(22, 33);
        TPoint b(22, 34);
        TPoint c(22, 32);
        TPoint d(33, 77);

        UNIT_ASSERT(a < b);
        UNIT_ASSERT(!(a < c));
        UNIT_ASSERT(a < d);
    }

    Y_UNIT_TEST(Cross) {
        TPoint a(7, 18);
        TPoint b(-118, 98);
        UNIT_ASSERT_EQUAL(7 * 98 + 18 * 118, a.Cross(b));
    }
}
