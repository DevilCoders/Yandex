#include <library/cpp/containers/vp_tree/static_vp_tree.h>
#include <library/cpp/testing/unittest/registar.h>

#include <cstdlib>

Y_UNIT_TEST_SUITE(TStaticVpTreeTest) {
    struct TPoint {
        double x;
        double y;
    };

    struct TMetric {
        double Distance(const TPoint& p1, const TPoint& p2) const {
            double dx = std::abs(p1.x - p2.x);
            double dy = std::abs(p1.y - p2.y);

            return Max(dx, dy);
        }
    };

    Y_UNIT_TEST(FindPoints) {
        TVector<TPoint> points(1000);

        for (size_t i = 0; i < points.size(); ++i) {
            TPoint p;
            p.x = i;
            p.y = i;
            points[i] = p;
        }

        TStaticVpTree<TPoint, TMetric> vpTree(points.begin(), points.end());
        TVector<const TPoint*> found;

        TPoint p;
        p.x = 17.3;
        p.y = 17.1;

        vpTree.FindNearbyItemsWithLimit(p, 0.08, found, 10);
        UNIT_ASSERT_VALUES_EQUAL(found.size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(vpTree.CountNearbyItems(p, 0.08), 0);

        vpTree.FindNearbyItemsWithLimit(p, 0.5, found, 10);
        UNIT_ASSERT_VALUES_EQUAL(found.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(found[0]->x, 17.);
        UNIT_ASSERT_VALUES_EQUAL(found[0]->y, 17.);
        UNIT_ASSERT_VALUES_EQUAL(vpTree.CountNearbyItems(p, 0.5), 1);

        found.clear();
        vpTree.FindNearbyItemsWithLimit(p, 3.5, found, 10);
        UNIT_ASSERT_VALUES_EQUAL(found.size(), 7);
        Sort(found.begin(), found.end());
        UNIT_ASSERT_VALUES_EQUAL(found[0]->x, 14.);
        UNIT_ASSERT_VALUES_EQUAL(found[1]->x, 15.);
        UNIT_ASSERT_VALUES_EQUAL(found[2]->x, 16.);
        UNIT_ASSERT_VALUES_EQUAL(found[3]->x, 17.);
        UNIT_ASSERT_VALUES_EQUAL(found[4]->x, 18.);
        UNIT_ASSERT_VALUES_EQUAL(found[5]->x, 19.);
        UNIT_ASSERT_VALUES_EQUAL(found[6]->x, 20.);
        UNIT_ASSERT_VALUES_EQUAL(vpTree.CountNearbyItems(p, 3.5), 7);

        found.clear();
        vpTree.FindNearbyItemsWithLimit(p, 3.5, found, 5);
        UNIT_ASSERT_VALUES_EQUAL(found.size(), 5);

        found.clear();
        auto&& fn = [&](const TPoint* pnt) {
            found.push_back(pnt);
            return true;
        };
        vpTree.FindNearbyItems(p, 5.5, fn);

        UNIT_ASSERT_VALUES_EQUAL(found.size(), 11);
        UNIT_ASSERT_VALUES_EQUAL(vpTree.CountNearbyItems(p, 5.5), 11);

        found.clear();
        vpTree.FindNearbyItemsWithLimit(p, 2000, found, 999);
        UNIT_ASSERT_VALUES_EQUAL(found.size(), 999);
        UNIT_ASSERT_VALUES_EQUAL(vpTree.CountNearbyItems(p, 2000), 1000);
    }

    Y_UNIT_TEST(EmptyTree) {
        TPoint points[1];
        TStaticVpTree<TPoint, TMetric> vpTree(points, points, TMetric());
        TPoint p;
        p.x = 1234.3;
        p.y = 671.1;

        TVector<const TPoint*> found;
        vpTree.FindNearbyItemsWithLimit(p, 1000., found, 10);
        UNIT_ASSERT_VALUES_EQUAL(found.size(), 0);
    }

    struct TEquidistantMetric {
        static int Distance(int p1, int p2) {
            return p1 == p2 ? 0 : 1;
        }
    };

    Y_UNIT_TEST(EquidistantItems) {
        TVector<int> items(1000);
        for (size_t i = 0; i < items.size(); ++i) {
            items[i] = i;
        }
        TStaticVpTree<int, TEquidistantMetric, int> vpTree(items.begin(), items.end());
        struct TCons {
            TVector<int> Result;
            bool operator()(const int* i) {
                Result.push_back(*i);
                return true;
            }
        } cons;
        vpTree.FindNearbyItems(777, 0, cons);

        UNIT_ASSERT_VALUES_EQUAL(cons.Result.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(cons.Result[0], 777);
    }
};
