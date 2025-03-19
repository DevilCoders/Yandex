#include "algorithms.h"
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TAlgorithmsSuite) {
    Y_UNIT_TEST(TestGroupingSimple) {
        DoInitGlobalLog("cout", TLOG_DEBUG, false, false);

        using TPoint = typename TRouteGroupingAlgorithm<TCoord<double>>::TPointWithLabel;
        {
            TGroupingAlgorithm<TPoint> alg(2, 1.5);
            alg.AddPoint(TPoint(TCoord<double>(0, 0), 0));
            alg.AddPoint(TPoint(TCoord<double>(0, 1), 1));
            alg.AddPoint(TPoint(TCoord<double>(1, 0), 2));
            alg.AddPoint(TPoint(TCoord<double>(1, 1), 3));

            alg.AddPoint(TPoint(TCoord<double>(2, 3), 4));
            alg.AddPoint(TPoint(TCoord<double>(3, 2), 5));
            alg.AddPoint(TPoint(TCoord<double>(3, 3), 6));

            typename TGroupingAlgorithm<TPoint>::TResult result = alg.Execute();
            UNIT_ASSERT_EQUAL(result.size(), 2);
            UNIT_ASSERT_EQUAL(result[0].size(), 4);
            UNIT_ASSERT_EQUAL(result[1].size(), 3);
        }
        {
            TGroupingAlgorithm<TPoint> alg(2, 1.5);
            alg.AddPoint(TPoint(TCoord<double>(0, 0), 0));
            alg.AddPoint(TPoint(TCoord<double>(0, 1), 1));
            alg.AddPoint(TPoint(TCoord<double>(1, 0), 2));
            alg.AddPoint(TPoint(TCoord<double>(1, 1), 3));
            alg.AddPoint(TPoint(TCoord<double>(2, 2), 4));
            alg.AddPoint(TPoint(TCoord<double>(2, 1), 5));
            alg.AddPoint(TPoint(TCoord<double>(1, 2), 6));
            alg.AddPoint(TPoint(TCoord<double>(2, 0), 7));
            alg.AddPoint(TPoint(TCoord<double>(0, 2), 8));

            typename TGroupingAlgorithm<TPoint>::TResult result = alg.Execute();
            UNIT_ASSERT_EQUAL(result.size(), 4);
            UNIT_ASSERT_EQUAL(result[0].size(), 4);
            UNIT_ASSERT_EQUAL(result[1].size(), 3);
            UNIT_ASSERT_EQUAL(result[2].size(), 1);
            UNIT_ASSERT_EQUAL(result[3].size(), 1);
        }
        {
            TGroupingAlgorithm<TPoint> alg(2, 1.5);
            alg.AddPoint(TPoint(TCoord<double>(0, 0), 0));
            typename TGroupingAlgorithm<TPoint>::TResult result = alg.Execute();
            UNIT_ASSERT_EQUAL(result.size(), 1);
            UNIT_ASSERT_EQUAL(result[0].size(), 1);
        }
        {
            TGroupingAlgorithm<TPoint> alg(2, 1.5);
            alg.AddPoint(TPoint(TCoord<double>(0, 0), 0));
            alg.AddPoint(TPoint(TCoord<double>(0, 0), 1));
            alg.AddPoint(TPoint(TCoord<double>(0, 0), 2));
            typename TGroupingAlgorithm<TPoint>::TResult result = alg.Execute();
            UNIT_ASSERT_EQUAL(result.size(), 1);
            UNIT_ASSERT_EQUAL(result[0].size(), 3);
        }
    }

    Y_UNIT_TEST(TestGroupingRoutes) {
        DoInitGlobalLog("cout", TLOG_DEBUG, false, false);
        {
            TRouteGroupingAlgorithm<TCoord<double>> alg(2, 1.5);
            TPolyLine<TCoord<double>> route = TPolyLine<TCoord<double>>::BuildFromString("0 0 3 3");
            alg.AddRoute(route);
            UNIT_ASSERT_EQUAL(alg.Execute().size(), 1);
            typename TRouteGroupingAlgorithm<TCoord<double>>::TClusters result = alg.GetClusters();
            UNIT_ASSERT_EQUAL(result.size(), 1);
        }
        {
            TRouteGroupingAlgorithm<TCoord<double>> alg(2, 1.5);
            TPolyLine<TCoord<double>> route1 = TPolyLine<TCoord<double>>::BuildFromString("0 3 4 3");
            TPolyLine<TCoord<double>> route2 = TPolyLine<TCoord<double>>::BuildFromString("0 4 4 2");
            TPolyLine<TCoord<double>> route3 = TPolyLine<TCoord<double>>::BuildFromString("0 0 4 0");
            TPolyLine<TCoord<double>> route4 = TPolyLine<TCoord<double>>::BuildFromString("1 4 2 1");
            alg.AddRoute(route1);
            alg.AddRoute(route2);
            alg.AddRoute(route3);
            alg.AddRoute(route4);
            UNIT_ASSERT_EQUAL(alg.Execute().size(), 4);
            typename TRouteGroupingAlgorithm<TCoord<double>>::TClusters result = alg.GetClusters();
            UNIT_ASSERT_EQUAL(result.size(), 3);
        }
        {
            TRouteGroupingAlgorithm<TCoord<double>> alg(2, 1.5);
            alg.AddRoute(TPolyLine<TCoord<double>>::BuildFromString("0 1 2 2"));
            alg.AddRoute(TPolyLine<TCoord<double>>::BuildFromString("0 0 2 2"));
            alg.AddRoute(TPolyLine<TCoord<double>>::BuildFromString("1 0 3 0"));
            alg.AddRoute(TPolyLine<TCoord<double>>::BuildFromString("3 4 2 2"));
            alg.AddRoute(TPolyLine<TCoord<double>>::BuildFromString("3 3 2 2"));
            alg.AddRoute(TPolyLine<TCoord<double>>::BuildFromString("2 4 4 3"));

            UNIT_ASSERT_EQUAL(alg.Execute().size(), 6);
            typename TRouteGroupingAlgorithm<TCoord<double>>::TClusters result = alg.GetClusters();
            UNIT_ASSERT_EQUAL(result.size(), 4);
        }
        {
            TRouteGroupingAlgorithm<TGeoCoord> alg(1000, 400);
            alg.AddRoute(TPolyLine<TGeoCoord>::BuildFromString("37.56834137826536 55.752160931563296 37.5843917170105 55.753310565639495"));
            alg.AddRoute(TPolyLine<TGeoCoord>::BuildFromString("37.56821263223265 55.75176157702661 37.58503544717407 55.752887020201776"));
            alg.AddRoute(TPolyLine<TGeoCoord>::BuildFromString("37.5843917170105 55.753310565639495 37.56216156869506 55.748046172894156"));
            alg.AddRoute(TPolyLine<TGeoCoord>::BuildFromString("37.58503544717407 55.752887020201776 37.563041333251945 55.747864629384644"));

            UNIT_ASSERT_EQUAL(alg.Execute().size(), 4);
            typename TRouteGroupingAlgorithm<TGeoCoord>::TClusters result = alg.GetClusters();
            UNIT_ASSERT_EQUAL(result.size(), 2);
            for (auto&& cluster: result) {
                UNIT_ASSERT_EQUAL(cluster.second.size(), 2);
                for (auto&& route: cluster.second) {
                    INFO_LOG << route.SerializeToString() << Endl;
                }
            }
        }
    }
}
