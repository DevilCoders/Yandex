#include "gen_geo_data.h"
#include "common.h"
#include "locations_converter.h"
#include "slab_handler.h"

#include <library/cpp/reverse_geocoder/library/log.h>
#include <library/cpp/reverse_geocoder/library/stop_watch.h>

#include <library/cpp/logger/global/global.h>

#include <util/generic/algorithm.h>

using namespace NReverseGeocoder;
using namespace NGenerator;

struct TCheckPoint {
    TCoordinate X;
    TNumber Idx;
    bool IsStart;
};

static TVector<TCheckPoint> MakeCheckPoints(const TVector<TEdge>& e,
                                            TGenGeoData* geoData) {
    TVector<TCheckPoint> checkPoints;
    checkPoints.reserve(e.size() * 2);

    const TPoint* p = geoData->Points();

    for (TNumber i = 0; i < e.size(); ++i) {
        checkPoints.push_back(TCheckPoint{p[e[i].Beg].X, i, true});
        checkPoints.push_back(TCheckPoint{p[e[i].End].X, i, false});
    }

    StableSort(checkPoints.begin(), checkPoints.end(), [&](const TCheckPoint& a, const TCheckPoint& b) {
        return a.X < b.X;
    });

    return checkPoints;
}

void NReverseGeocoder::NGenerator::TSlabHandler::Update(TGeoId regionId, TGeoId polygonId, const TVector<TPoint>& points,
                                                        TPolygon::EType type) {
    if (points.size() <= 2) {
        LogWarning("Polygon %lu too small (region %lu)", polygonId, regionId);
        return;
    }

    TPolygon polygon;
    memset(&polygon, 0, sizeof(polygon));

    polygon.RegionId = regionId;
    polygon.PolygonId = polygonId;
    polygon.Square = GetSquare(points);
    polygon.Type = type;
    polygon.Bbox = TBoundingBox(points.data(), points.size());
    polygon.PointsNumber = points.size();

    polygon.PartsOffset = GeoData_->PartsNumber();

    const TVector<TEdge> edges = MakeEdges(points, GeoData_);
    const TVector<TCheckPoint> checkPoints = MakeCheckPoints(edges, GeoData_);

    for (TNumber l = 0, r = 0; l < checkPoints.size(); l = r) {
        r = l + 1;
        while (r < checkPoints.size() && checkPoints[l].X == checkPoints[r].X)
            ++r;

        TVector<TEdge> erase;
        erase.reserve(r - l);

        for (TNumber i = l; i < r; ++i)
            if (!checkPoints[i].IsStart)
                erase.push_back(edges[checkPoints[i].Idx]);

        StableSort(erase.begin(), erase.end());

        TPart part;
        part.EdgeRefsOffset = GeoData_->EdgeRefsNumber();
        part.Coordinate = checkPoints[l].X;

        if (l > 0) {
            const TPart& prev = *(GeoData_->Parts() + GeoData_->PartsNumber() - 1);
            for (TNumber i = prev.EdgeRefsOffset; i < part.EdgeRefsOffset; ++i) {
                const TEdge& e = GeoData_->Edges()[GeoData_->EdgeRefs()[i]];
                if (!BinarySearch(erase.begin(), erase.end(), e))
                    GeoData_->EdgeRefsAppend(GeoData_->EdgeRefs()[i]);
            }
        }

        for (TNumber i = l; i < r; ++i)
            if (checkPoints[i].IsStart)
                if (!BinarySearch(erase.begin(), erase.end(), edges[checkPoints[i].Idx]))
                    GeoData_->EdgeRefsAppend(GeoData_->Insert(edges[checkPoints[i].Idx]));

        TRef* mutEdgeRefs = GeoData_->MutEdgeRefs() + part.EdgeRefsOffset;
        TRef* mutEdgeRefsEnd = GeoData_->MutEdgeRefs() + GeoData_->EdgeRefsNumber();

        StableSort(mutEdgeRefs, mutEdgeRefsEnd, [&](const TRef& a, const TRef& b) {
            return GeoData_->Edges()[a].Lower(GeoData_->Edges()[b], GeoData_->Points());
        });

        GeoData_->PartsAppend(part);
    }

    polygon.PartsNumber = GeoData_->PartsNumber() - polygon.PartsOffset;
    GeoData_->PolygonsAppend(polygon);
}

void NReverseGeocoder::NGenerator::TSlabHandler::Init() {
    // INFO_LOG << "Generate polygons..." << Endl;

    StopWatch_.Run();
}

static void SortPolygons(TGenGeoData* g) {
    TPolygon* begin = g->MutPolygons();
    TPolygon* end = begin + g->PolygonsNumber();

    StableSort(begin, end, [](const TPolygon& a, const TPolygon& b) {
        return a.Bbox.X2 < b.Bbox.X2;
    });
}

void NReverseGeocoder::NGenerator::TSlabHandler::GenerateAreaBoxes() {
    // INFO_LOG << "Generate area boxes..." << Endl;

    TStopWatch stopWatch;
    stopWatch.Run();

    SortPolygons(GeoData_);

    TVector<TVector<TRef>> areaBoxes(NAreaBox::Number);

    TVector<TBoundingBox> bboxes;
    bboxes.reserve(NAreaBox::Number);

    for (TCoordinate x0 = NAreaBox::LowerX; x0 < NAreaBox::UpperX; x0 += NAreaBox::DeltaX)
        for (TCoordinate y0 = NAreaBox::LowerY; y0 < NAreaBox::UpperY; y0 += NAreaBox::DeltaY)
            bboxes.emplace_back(x0, y0, x0 + NAreaBox::DeltaX, y0 + NAreaBox::DeltaY);

    const TNumber polygonsNumber = GeoData_->PolygonsNumber();
    const TPolygon* polygons = GeoData_->Polygons();

    for (TNumber i = 0; i < polygonsNumber; ++i) {
        const TCoordinate x1 = polygons[i].Bbox.X1;
        const TCoordinate y1 = polygons[i].Bbox.Y1;
        const TCoordinate x2 = polygons[i].Bbox.X2;
        const TCoordinate y2 = polygons[i].Bbox.Y2;
        for (TCoordinate x0 = x1; x0 <= x2 + NAreaBox::DeltaX; x0 += NAreaBox::DeltaX) {
            for (TCoordinate y0 = y1; y0 <= y2 + NAreaBox::DeltaY; y0 += NAreaBox::DeltaY) {
                const TRef box = LookupAreaBox(TPoint(x0, y0));
                if (box >= bboxes.size() || box >= areaBoxes.size())
                    continue;
                if (polygons[i].Bbox.HasIntersection(bboxes[box]))
                    areaBoxes[box].push_back(i);
            }
        }
    }

    TRef areaBoxRef = 0;
    for (TCoordinate x0 = NAreaBox::LowerX; x0 < NAreaBox::UpperX; x0 += NAreaBox::DeltaX) {
        for (TCoordinate y0 = NAreaBox::LowerY; y0 < NAreaBox::UpperY; y0 += NAreaBox::DeltaY) {
            TAreaBox box;
            box.PolygonRefsOffset = GeoData_->PolygonRefsNumber();

            for (const TRef i : areaBoxes[areaBoxRef])
                GeoData_->PolygonRefsAppend(i);

            box.PolygonRefsNumber = GeoData_->PolygonRefsNumber() - box.PolygonRefsOffset;

            TRef* mutPolygonRefs = GeoData_->MutPolygonRefs() + box.PolygonRefsOffset;
            TRef* mutPolygonRefsEnd = GeoData_->MutPolygonRefs() + GeoData_->PolygonRefsNumber();

            auto cmp = [&](const TRef& a, const TRef& b) {
                return polygons[a].RegionId < polygons[b].RegionId || (polygons[a].RegionId == polygons[b].RegionId && polygons[a].Square < polygons[b].Square);
            };

            StableSort(mutPolygonRefs, mutPolygonRefsEnd, cmp);

            GeoData_->BoxesAppend(box);

            ++areaBoxRef;
        }
    }

    // INFO_LOG << "Area boxes generated in " << stopWatch.Get() << " seconds" << Endl;
}

// Create fake part for right edgeRefsNumber calculation.
static void CreateFakePart(TGenGeoData* geoData) {
    TPart fakePart;
    fakePart.Coordinate = 0;
    fakePart.EdgeRefsOffset = geoData->EdgeRefsNumber();
    geoData->PartsAppend(fakePart);
}

void NReverseGeocoder::NGenerator::TSlabHandler::Fini() {
    // INFO_LOG << "Polygons generated in " << StopWatch_.Get() << " seconds" << Endl;

    GenerateAreaBoxes();
    CreateFakePart(GeoData_);

    GeoData_->SetVersion(GEO_DATA_CURRENT_VERSION);
}

void NReverseGeocoder::NGenerator::TSlabHandler::Update(TGeoId regionId, const NProto::TPolygon& polygon) {
    TLocationsConverter converter;
    converter.Each(polygon.GetLocations(), [&](const TVector<TLocation>& locations) {
        TVector<TPoint> points;
        points.reserve(locations.size());
        for (const TLocation& l : locations)
            points.push_back(TPoint(l));
        Update(regionId, polygon.GetPolygonId(), points, (TPolygon::EType)polygon.GetType());
    });
}

void NReverseGeocoder::NGenerator::TSlabHandler::Update(const NProto::TRegion& protoRegion) {
    for (const NProto::TPolygon& polygon : protoRegion.GetPolygons())
        Update(protoRegion.GetRegionId(), polygon);
}
