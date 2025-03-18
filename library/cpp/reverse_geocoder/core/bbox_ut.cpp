#include <library/cpp/testing/unittest/registar.h>

#include "bbox.h"

using namespace NReverseGeocoder;

Y_UNIT_TEST_SUITE(TBoundingBox) {
    Y_UNIT_TEST(TestDefaultConstruct) {
        TBoundingBox bbox;

        UNIT_ASSERT_EQUAL(0, bbox.X1);
        UNIT_ASSERT_EQUAL(0, bbox.Y1);
        UNIT_ASSERT_EQUAL(0, bbox.X2);
        UNIT_ASSERT_EQUAL(0, bbox.Y2);
    }

    Y_UNIT_TEST(TestCoordinateConstructor) {
        TCoordinate X1 = 11;
        TCoordinate Y1 = 78;
        TCoordinate X2 = 78787878;
        TCoordinate Y2 = -909090;

        TBoundingBox bbox(X1, Y1, X2, Y2);

        UNIT_ASSERT_EQUAL(X1, bbox.X1);
        UNIT_ASSERT_EQUAL(Y1, bbox.Y1);
        UNIT_ASSERT_EQUAL(X2, bbox.X2);
        UNIT_ASSERT_EQUAL(Y2, bbox.Y2);
    }

    Y_UNIT_TEST(TestPointsConstructor) {
        TVector<TPoint> points = {
            TPoint(0, 0),
            TPoint(4, 0),
            TPoint(5, 3),
            TPoint(0, 7),
        };

        TBoundingBox bbox(points.data(), points.size());

        UNIT_ASSERT_EQUAL(0, bbox.X1);
        UNIT_ASSERT_EQUAL(0, bbox.Y1);
        UNIT_ASSERT_EQUAL(5, bbox.X2);
        UNIT_ASSERT_EQUAL(7, bbox.Y2);
    }

    Y_UNIT_TEST(TestInit) {
        TBoundingBox bbox;
        bbox.Init();

        UNIT_ASSERT_EQUAL(ToCoordinate(180.0), bbox.X1);
        UNIT_ASSERT_EQUAL(ToCoordinate(90.0), bbox.Y1);
        UNIT_ASSERT_EQUAL(ToCoordinate(-180.0), bbox.X2);
        UNIT_ASSERT_EQUAL(ToCoordinate(-90.0), bbox.Y2);
    }

    Y_UNIT_TEST(TestHasIntersection) {
        TVector<TPoint> points1 = {
            TPoint(0, 0),
            TPoint(4, 0),
            TPoint(5, 3),
            TPoint(0, 7),
        };

        TVector<TPoint> points2 = {
            TPoint(4, 0),
            TPoint(6, 0),
            TPoint(6, 3),
            TPoint(4, 3),
        };

        TVector<TPoint> points3 = {
            TPoint(-1, 0),
            TPoint(-1, -1),
            TPoint(-2, -1),
            TPoint(-2, -2),
        };

        TBoundingBox bbox1(points1.data(), points1.size());
        TBoundingBox bbox2(points2.data(), points2.size());
        TBoundingBox bbox3(points3.data(), points3.size());

        UNIT_ASSERT(bbox1.HasIntersection(bbox2));
        UNIT_ASSERT(!bbox1.HasIntersection(bbox3));
    }

    Y_UNIT_TEST(TestContainsPoint) {
        TVector<TPoint> points = {
            TPoint(0, 0),
            TPoint(4, 0),
            TPoint(5, 3),
            TPoint(0, 7),
        };

        TBoundingBox bbox(points.data(), points.size());

        UNIT_ASSERT(bbox.Contains(TPoint(0, 0)));
        UNIT_ASSERT(bbox.Contains(TPoint(1, 1)));
        UNIT_ASSERT(bbox.Contains(TPoint(0, 2)));
        UNIT_ASSERT(bbox.Contains(TPoint(5, 7)));
        UNIT_ASSERT(bbox.Contains(TPoint(5, 3)));

        UNIT_ASSERT(!bbox.Contains(TPoint(-1, 0)));
        UNIT_ASSERT(!bbox.Contains(TPoint(5, 8)));
        UNIT_ASSERT(!bbox.Contains(TPoint(8, 5)));
        UNIT_ASSERT(!bbox.Contains(TPoint(0, 1000)));
    }
}
