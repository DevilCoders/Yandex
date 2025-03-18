#include <library/cpp/testing/unittest/registar.h>

#include "edge.h"

#include <library/cpp/reverse_geocoder/test/unittest/unittest.h>

using namespace NReverseGeocoder;

Y_UNIT_TEST_SUITE(TEdge) {
    Y_UNIT_TEST(DefaultConstructor) {
        TEdge e;
        UNIT_ASSERT_EQUAL(0, e.Beg);
        UNIT_ASSERT_EQUAL(0, e.End);
    }

    Y_UNIT_TEST(AssignmentConstructor) {
        TEdge a(21, 22);
        TEdge b(a);

        UNIT_ASSERT_EQUAL(a.Beg, b.Beg);
        UNIT_ASSERT_EQUAL(a.End, b.End);
        UNIT_ASSERT_EQUAL(21, b.Beg);
        UNIT_ASSERT_EQUAL(22, b.End);
    }

    Y_UNIT_TEST(Constructor) {
        TEdge e(22, 79);
        UNIT_ASSERT_EQUAL(22, e.Beg);
        UNIT_ASSERT_EQUAL(79, e.End);
    };

    Y_UNIT_TEST(OperatorEquals) {
        TEdge a(22, 79);
        TEdge b(22, 79);
        TEdge c(21, 79);

        UNIT_ASSERT(a == b);
        UNIT_ASSERT(!(a == c));
    }

    Y_UNIT_TEST(OperatorNotEquals) {
        TEdge a(22, 79);
        TEdge b(22, 79);
        TEdge c(21, 79);

        UNIT_ASSERT(!(a != b));
        UNIT_ASSERT(a != c);
    }

    Y_UNIT_TEST(OperatorLess) {
        TEdge a(22, 79);
        TEdge b(21, 100);
        TEdge c(22, 100);

        UNIT_ASSERT(!(a < b));
        UNIT_ASSERT(b < a);
        UNIT_ASSERT(a < c);
        UNIT_ASSERT(!(c < a));
    }

    Y_UNIT_TEST(ContainsPoint) {
        TEdgeMaker maker;
        TEdge a = maker.MakeEdge(TPoint(0, 0), TPoint(2, 2));

        UNIT_ASSERT(a.Contains(TPoint(0, 0), maker.Points()));
        UNIT_ASSERT(a.Contains(TPoint(1, 1), maker.Points()));
        UNIT_ASSERT(a.Contains(TPoint(2, 2), maker.Points()));

        UNIT_ASSERT(!a.Contains(TPoint(-1, -1), maker.Points()));
        UNIT_ASSERT(!a.Contains(TPoint(3, 3), maker.Points()));
        UNIT_ASSERT(!a.Contains(TPoint(2, 1), maker.Points()));
    }

    Y_UNIT_TEST(EdgeLower) {
        TEdgeMaker maker;

        TEdge a = maker.MakeEdge(TPoint(0, 0), TPoint(2, 1));
        TEdge b = maker.MakeEdge(TPoint(0, 0), TPoint(2, 2));
        TEdge c = maker.MakeEdge(TPoint(1, 3), TPoint(5, 5));
        TEdge d = maker.MakeEdge(TPoint(0, 4), TPoint(5, 5));

        UNIT_ASSERT(a.Lower(b, maker.Points()));
        UNIT_ASSERT(a.Lower(c, maker.Points()));
        UNIT_ASSERT(a.Lower(d, maker.Points()));
        UNIT_ASSERT(b.Lower(c, maker.Points()));
        UNIT_ASSERT(b.Lower(d, maker.Points()));
        UNIT_ASSERT(c.Lower(d, maker.Points()));

        UNIT_ASSERT(!d.Lower(a, maker.Points()));
        UNIT_ASSERT(!d.Lower(b, maker.Points()));
        UNIT_ASSERT(!d.Lower(c, maker.Points()));
        UNIT_ASSERT(!c.Lower(b, maker.Points()));
        UNIT_ASSERT(!c.Lower(a, maker.Points()));
        UNIT_ASSERT(!b.Lower(a, maker.Points()));

        TEdge a1 = maker.MakeEdge(TPoint(0, 2), TPoint(3, 3));
        TEdge a2 = maker.MakeEdge(TPoint(0, 0), TPoint(1, 2));
        TEdge a3 = maker.MakeEdge(TPoint(0, 0), TPoint(3, 3));

        UNIT_ASSERT(a3.Lower(a2, maker.Points()));
        UNIT_ASSERT(a3.Lower(a1, maker.Points()));
        UNIT_ASSERT(a2.Lower(a1, maker.Points()));

        UNIT_ASSERT(!a2.Lower(a3, maker.Points()));
        UNIT_ASSERT(!a1.Lower(a3, maker.Points()));
        UNIT_ASSERT(!a1.Lower(a2, maker.Points()));

        TEdge a4 = maker.MakeEdge(TPoint(2, 5), TPoint(3, 7));
        TEdge a5 = maker.MakeEdge(TPoint(2, 5), TPoint(4, 6));
        TEdge a6 = maker.MakeEdge(TPoint(2, 4), TPoint(4, 6));
        TEdge a11 = maker.MakeEdge(TPoint(3, 7), TPoint(10, 5));

        UNIT_ASSERT(a6.Lower(a5, maker.Points()));
        UNIT_ASSERT(a6.Lower(a4, maker.Points()));
        UNIT_ASSERT(a6.Lower(a11, maker.Points()));
        UNIT_ASSERT(a5.Lower(a4, maker.Points()));
        UNIT_ASSERT(a5.Lower(a11, maker.Points()));

        UNIT_ASSERT(!a5.Lower(a6, maker.Points()));
        UNIT_ASSERT(!a4.Lower(a6, maker.Points()));
        UNIT_ASSERT(!a11.Lower(a6, maker.Points()));
        UNIT_ASSERT(!a4.Lower(a5, maker.Points()));
        UNIT_ASSERT(!a11.Lower(a5, maker.Points()));

        TEdge a7 = maker.MakeEdge(TPoint(4, 4), TPoint(5, 5));
        TEdge a8 = maker.MakeEdge(TPoint(5, 5), TPoint(6, 4));
        TEdge a9 = maker.MakeEdge(TPoint(5, 4), TPoint(6, 3));
        TEdge a10 = maker.MakeEdge(TPoint(4, 4), TPoint(6, 3));

        UNIT_ASSERT(a10.Lower(a9, maker.Points()));
        UNIT_ASSERT(a10.Lower(a7, maker.Points()));
        UNIT_ASSERT(a10.Lower(a8, maker.Points()));
        UNIT_ASSERT(a9.Lower(a8, maker.Points()));

        UNIT_ASSERT(!a9.Lower(a10, maker.Points()));
        UNIT_ASSERT(!a7.Lower(a10, maker.Points()));
        UNIT_ASSERT(!a8.Lower(a10, maker.Points()));
        UNIT_ASSERT(!a8.Lower(a9, maker.Points()));
    }
}
