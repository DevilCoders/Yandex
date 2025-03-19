#pragma once

#include "coord.h"
#include "polyline.h"

#include <library/cpp/logger/global/global.h>
#include <util/generic/set.h>
#include <util/digest/fnv.h>

constexpr double OperationPrecision = 1e-5;

template <class TCoord>
inline double Angle(const TCoord& left, const TCoord& center, const TCoord& right) {
    double scalar;
    double vector;
    center.template GetProducts<true>(left, right, scalar, vector);
    return atan2(vector, scalar);
}

template <class TCoord>
inline double Angle(const TGeoCoord& center, const TVector<TGeoCoord>& path) {
    double result = 0;
    for (size_t i = 0; (i + 1) < path.size(); ++i) {
        result += Angle(path[i], center, path[i + 1]);
    }
    return result;
}

template <class TCoord>
inline void Append(TVector<TCoord>& target, const TVector<TCoord>& source, double precision = OperationPrecision) {
    target.reserve(target.size() + source.size());
    for (auto&& c : source) {
        if (target.empty() || target.back().GetLengthTo(c) > precision) {
            target.push_back(c);
        }
    }
}

template <class T, class TTo = T>
TTo BuildLinearApproximation(const TVector<T>& data, const double idxDouble) {
    CHECK_WITH_LOG(idxDouble >= 0 && idxDouble <= data.size() - 1 + 1e-5) << idxDouble << " / " << data.size();
    ui32 idx = (ui32)idxDouble;
    double delta = idxDouble - idx;
    if (delta > 1e-5) {
        CHECK_WITH_LOG(idx + 1 < data.size()) << idxDouble << "/" << data.size();
        return data[idx] * (1 - delta) + data[idx + 1] * delta;
    } else {
        CHECK_WITH_LOG(idx < data.size()) << idxDouble << "/" << data.size();
        return data[idx];
    }
}

template <class TCoord>
class TGroupingAlgorithm {
private:
    static const ui32 UNKNOWN_CLUSTER = Max<ui32>();

    struct TPointState {
        double Distance = Max<double>();
        double Diameter = 0;
        ui32 Cluster = UNKNOWN_CLUSTER;
        bool Visited = false;
    };

public:
    using TResult = TVector<TVector<TCoord>>;

    TGroupingAlgorithm(double diameter, double maxDistance)
        : Diameter(diameter)
        , MaxDistance(maxDistance)
    {}

    TGroupingAlgorithm& AddPoint(const TCoord& point) {
        Points.push_back(point);
        return *this;
    }

    TResult Execute() {
        Init();
        TResult result;

        for (CurrentCluster = 0; true; ++CurrentCluster) {
            ui32 start = GetPendingPoint();

            if (start == Max<ui32>())
                break;

            DEBUG_LOG << "Start cluster construction from " << Points[start].ToString() << Endl;
            result.push_back(TVector<TCoord>());

            InitStep();

            States[start].Distance = 0;

            for(ui32 nextPoint = start; nextPoint != Max<ui32>() && States[nextPoint].Distance < MaxDistance;
                     nextPoint = UpdateDistances(Points[nextPoint])) {

                DEBUG_LOG << "Get next " << Points[nextPoint].ToString() << Endl;

                States[nextPoint].Visited = true;

                double diameter = CalcDiameters(Points[nextPoint], CurrentCluster, false);
                DEBUG_LOG << "Calc diameter for " << CurrentCluster << ": " << diameter << Endl;
                if (diameter > Diameter) {
                    continue;
                }

                DEBUG_LOG << "Add point " << Points[nextPoint].ToString() << Endl;
                States[nextPoint].Cluster = CurrentCluster;
                CalcDiameters(Points[nextPoint], CurrentCluster, true);
                result.back().push_back(Points[nextPoint]);
            }
        }

        DEBUG_LOG << CurrentCluster << " clusters found" << Endl;
        return result;
    }

private:
    void Init() {
        CurrentCluster = 0;
        States.resize(Points.size());
    }

    void InitStep() {
        for (ui32 i = 0; i < States.size(); ++i) {
            States[i].Distance = Max<double>();
            States[i].Diameter = 0;
            if (States[i].Cluster == UNKNOWN_CLUSTER)
                States[i].Visited = false;
        }
    }

    ui32 GetPendingPoint() const {
        for (ui32 i = 0; i < States.size(); ++i) {
            if (States[i].Cluster == UNKNOWN_CLUSTER) {
                return i;
            }
        }
        return Max<ui32>();
    }

    ui32 UpdateDistances(TCoord newPoint) {
        ui32 nearestPoint = Max<ui32>();
        double nearestDist = Max<double>();

        for (ui32 i = 0; i < States.size(); ++i) {
            if (States[i].Visited)
                continue;

            States[i].Distance = Min(States[i].Distance, newPoint.GetLengthTo(Points[i]));
            if (States[i].Distance < nearestDist && Points[i].GetLabel() != newPoint.GetLabel()) {
                nearestPoint = i;
                nearestDist = States[i].Distance;
            }
        }

        return nearestPoint;
    }

    double CalcDiameters(TCoord newPoint, ui32 cluster, bool update) {
        double diameter = 0;

        for (ui32 i = 0; i < States.size(); ++i) {
            double newDiameter = Max(States[i].Diameter, newPoint.GetLengthTo(Points[i]));

            if (States[i].Cluster == cluster && newDiameter > diameter) {
                diameter = newDiameter;
            }

            if (update) {
                States[i].Diameter = newDiameter;
            }
        }

        return diameter;
    }

private:
    TVector<TCoord> Points;
    double Diameter;
    double MaxDistance;

    ui32 CurrentCluster;
    TVector<TPointState> States;
};


template <class TPoint>
class TRouteGroupingAlgorithm {
public:
    class TPointWithLabel : public TPoint {
    public:
        TPointWithLabel(const TPoint& point, ui64 label)
            : TPoint(point)
            , Label(label)
        {}

        ui64 GetLabel() const {
            return Label;
        }

    private:
        ui64 Label;
    };

    struct TRouteInfo {
        TPolyLine<TPoint> Route;
        ui64 ClusterFrom = Max<ui64>();
        ui64 ClusterTo = Max<ui64>();

        TRouteInfo(const TPolyLine<TPoint>& route)
            : Route(route)
        {}

        ui64 GetCluster() const {
            CHECK_WITH_LOG(ClusterFrom != Max<ui64>());
            CHECK_WITH_LOG(ClusterTo != Max<ui64>());
            return FnvHash<ui64>(ToString());
        }

        TString ToString() const {
            return "from=" + ::ToString(ClusterFrom) + ";to=" + ::ToString(ClusterTo);
        }
    };

public:
    using TClusters = THashMap<ui64, TVector<TPolyLine<TPoint>>>;

    TRouteGroupingAlgorithm(double diameter, double maxDistance)
        : StartGroups(diameter, maxDistance)
        , FinishGroups(diameter, maxDistance)
    {}

    void AddRoute(const TPolyLine<TPoint>& route) {
        ui32 routeId = Routes.size();
        INFO_LOG << "Add route start=" << route.FirstCoord().ToString() << ";finish=" << route.LastCoord().ToString() << Endl;
        StartGroups.AddPoint(TPointWithLabel(route.FirstCoord(), routeId));
        FinishGroups.AddPoint(TPointWithLabel(route.LastCoord(), routeId));
        Routes.push_back(TRouteInfo(route));
    }

    const TVector<TRouteInfo>& Execute() {
        DEBUG_LOG << "Start groups clustering..." << Endl;
        typename TGroupingAlgorithm<TPointWithLabel>::TResult startGroups = StartGroups.Execute();
        DEBUG_LOG << "Start groups clustering...OK" << Endl;

        for (ui32 group = 0; group < startGroups.size(); ++group) {
            for (ui32 j = 0; j < startGroups[group].size(); ++j) {
                ui32 routeId = startGroups[group][j].GetLabel();
                Routes[routeId].ClusterFrom = group;
            }
        }

        DEBUG_LOG << "Finish groups clustering..." << Endl;
        typename TGroupingAlgorithm<TPointWithLabel>::TResult finishGroups = FinishGroups.Execute();
        DEBUG_LOG << "Finish groups clustering...OK" << Endl;

        for (ui32 fGroup = 0; fGroup < finishGroups.size(); ++fGroup) {
            for (ui32 j = 0; j < finishGroups[fGroup].size(); ++j) {
                ui32 routeId = finishGroups[fGroup][j].GetLabel();
                CHECK_WITH_LOG(Routes[routeId].ClusterFrom != Max<ui64>());
                Routes[routeId].ClusterTo = fGroup;
            }
        }

        return Routes;
    }

    TClusters GetClusters() const {
        TClusters clusters;

        for (ui32 i = 0; i < Routes.size(); ++i) {
            clusters[Routes[i].GetCluster()].push_back(Routes[i].Route);
        }

        return clusters;
    }

private:
    TGroupingAlgorithm<TPointWithLabel> StartGroups;
    TGroupingAlgorithm<TPointWithLabel> FinishGroups;
    TVector<TRouteInfo> Routes;
};
