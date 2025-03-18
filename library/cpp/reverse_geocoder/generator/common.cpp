#include "common.h"

#include <library/cpp/reverse_geocoder/library/log.h>

#include <util/generic/ymath.h>

using namespace NReverseGeocoder;
using namespace NGenerator;

TSquare NReverseGeocoder::NGenerator::GetSquare(const TVector<TPoint>& p) {
    TSquare s = 0;
    for (TNumber i = 0; i < p.size(); ++i) {
        TNumber j = (i + 1 == p.size() ? 0 : i + 1);
        s += p[i].Cross(p[j]);
    }
    return s > 0 ? s : -s;
}

static bool IsBadEdge(const TEdge& e, const TPoint* p) {
    return Abs(p[e.Beg].X - p[e.End].X) > ToCoordinate(300.0);
}

TVector<TEdge> NReverseGeocoder::NGenerator::MakeEdges(const TVector<TPoint>& points, TGenGeoData* geoData,
                                                       bool changeDirection) {
    TVector<TEdge> edges;
    edges.reserve(points.size());

    for (TNumber i = 0; i < points.size(); ++i) {
        TNumber j = (i + 1 == points.size() ? 0 : i + 1);

        if (points[i] == points[j])
            continue;

        const TRef& p1 = geoData->Insert(points[i]);
        const TRef& p2 = geoData->Insert(points[j]);
        TEdge e(p1, p2);

        const TPoint* p = geoData->Points();
        if (p[e.Beg].X > p[e.End].X && changeDirection) {
            const TRef beg = e.Beg;
            e.Beg = e.End;
            e.End = beg;
        }

        if (IsBadEdge(e, p)) {
            LogWarning("Bad edge (%f, %f)=>(%f, %f)",
                       ToDouble(p[e.Beg].X), ToDouble(p[e.Beg].Y),
                       ToDouble(p[e.End].X), ToDouble(p[e.End].Y));
            continue;
        }

        edges.push_back(e);
    }

    return edges;
}
