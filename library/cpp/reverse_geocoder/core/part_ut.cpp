#include <library/cpp/testing/unittest/registar.h>

#include "part.h"

#include <library/cpp/reverse_geocoder/test/unittest/unittest.h>

using namespace NReverseGeocoder;

Y_UNIT_TEST_SUITE(TPart) {
    Y_UNIT_TEST(TwoEdges) {
        TPartMaker maker;
        TPartHelper part = maker.MakePart(0, {maker.MakeEdge(TPoint(0, 0), TPoint(3, 0)),
                                              maker.MakeEdge(TPoint(0, 3), TPoint(5, 5))});

        UNIT_ASSERT_EQUAL(0, part.Coordinate);
        UNIT_ASSERT(part.Contains(TPoint(1, 1), maker));
    }

    Y_UNIT_TEST(Contains) {
        TPartMaker maker;
        TPartHelper part = maker.MakePart(0, {maker.MakeEdge(TPoint(0, 0), TPoint(3, 0)),
                                              maker.MakeEdge(TPoint(0, 3), TPoint(3, 0)),
                                              maker.MakeEdge(TPoint(-1, 10), TPoint(10, 10)),
                                              maker.MakeEdge(TPoint(-1, 10), TPoint(10, 100))});

        UNIT_ASSERT(part.Contains(TPoint(1, 1), maker));
        UNIT_ASSERT(part.Contains(TPoint(3, 0), maker));
        UNIT_ASSERT(part.Contains(TPoint(0, 11), maker));
        UNIT_ASSERT(part.Contains(TPoint(0, 3), maker));

        UNIT_ASSERT(!part.Contains(TPoint(2, 5), maker));
        UNIT_ASSERT(!part.Contains(TPoint(2, -1), maker));
        UNIT_ASSERT(!part.Contains(TPoint(2, 101), maker));
    }
}
