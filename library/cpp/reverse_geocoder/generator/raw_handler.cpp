#include "common.h"
#include "locations_converter.h"
#include "raw_handler.h"

#include <library/cpp/reverse_geocoder/library/log.h>

#include <util/generic/algorithm.h>

using namespace NReverseGeocoder;
using namespace NGenerator;

void NReverseGeocoder::NGenerator::TRawHandler::Init() {
    if (!Config_.SaveRawBorders)
        return;
}

void NReverseGeocoder::NGenerator::TRawHandler::Update(TGeoId regionId, TGeoId polygonId,
                                                       const TVector<TPoint>& points, TRawPolygon::EType type) {
    if (points.size() <= 2) {
        LogWarning("Polygon %lu too small (region %lu)", polygonId, regionId);
        return;
    }

    TRawPolygon border;
    memset(&border, 0, sizeof(border));

    border.RegionId = regionId;
    border.PolygonId = polygonId;
    border.Square = GetSquare(points);
    border.Type = type;
    border.Bbox = TBoundingBox(points.data(), points.size());

    border.EdgeRefsOffset = GeoData_->RawEdgeRefsNumber();

    const TVector<TEdge> edges = MakeEdges(points, GeoData_, /* changeDirection = */ false);

    for (TNumber i = 0; i < edges.size(); ++i)
        GeoData_->RawEdgeRefsAppend(GeoData_->Insert(edges[i]));

    border.EdgeRefsNumber = GeoData_->RawEdgeRefsNumber() - border.EdgeRefsOffset;
    GeoData_->RawPolygonsAppend(border);
}

void NReverseGeocoder::NGenerator::TRawHandler::Update(TGeoId regionId, const NProto::TPolygon& polygon) {
    TLocationsConverter converter;
    converter.Each(polygon.GetLocations(), [&](const TVector<TLocation>& locations) {
        TVector<TPoint> points;
        points.reserve(locations.size());
        for (const TLocation& l : locations)
            points.push_back(TPoint(l));
        Update(regionId, polygon.GetPolygonId(), points, (TRawPolygon::EType)polygon.GetType());
    });
}

void NReverseGeocoder::NGenerator::TRawHandler::Update(const NProto::TRegion& region) {
    if (!Config_.SaveRawBorders)
        return;

    for (const NProto::TPolygon& polygon : region.GetPolygons())
        Update(region.GetRegionId(), polygon);
}

void NReverseGeocoder::NGenerator::TRawHandler::Fini() {
    if (!Config_.SaveRawBorders)
        return;

    TRawPolygon* beg = GeoData_->MutRawPolygons();
    TRawPolygon* end = beg + GeoData_->RawPolygonsNumber();

    StableSort(beg, end, [](const TRawPolygon& a, const TRawPolygon& b) {
        return a.RegionId < b.RegionId || (a.RegionId == b.RegionId && a.Square < b.Square);
    });
}
