#include <library/cpp/testing/benchmark/bench.h>
#include <library/cpp/containers/vp_tree/static_vp_tree.h>

#include <cmath>
#include <cstdlib>

struct TPoint {
    double x;
    double y;
};

struct TMetric {
    inline double Distance(const TPoint& p1, const TPoint& p2) const {
        double dx = fabs(p1.x - p2.x);
        double dy = fabs(p1.y - p2.y);

        return Max(dx, dy);
    }
};

Y_CPU_BENCHMARK(VPTree, iface) {
    const auto N = iface.Iterations();

    TVector<TPoint> points(10000);

    for (size_t i = 0; i < points.size(); ++i) {
        TPoint p;
        p.x = i;
        p.y = i;
        points[i] = p;
    }

    TStaticVpTree<TPoint, TMetric> vpTree(points.begin(), points.end(), TMetric());
    TVector<const TPoint*> found;

    for (size_t i = 0; i < N; ++i) {
        TPoint p;

        p.x = (i % points.size()) + 0.3;
        p.y = (i % points.size()) + 0.1;

        found.clear();
        vpTree.FindNearbyItemsWithLimit(p, 3.5, found, 10);
    }

    Y_DO_NOT_OPTIMIZE_AWAY(found);
}
