#include "tile.h"

#include <util/generic/deque.h>
#include <util/generic/set.h>
#include <util/generic/xrange.h>

#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/CoordinateSequenceFactory.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/util/GeometricShapeFactory.h>
#include <geos/util/TopologyException.h>

using namespace NGeoHash;

enum TAngle {
    BOTTOM_LEFT = 1,
    BOTTOM_RIGHT = 2,
    TOP_LEFT = 4,
    TOP_RIGHT = 8
};

const TAngle angles[] = {
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    TOP_LEFT,
    TOP_RIGHT};

const TNeighbours<char> angleMasks = {
    TAngle::BOTTOM_LEFT | TAngle::BOTTOM_RIGHT, // NORTH
    TAngle::BOTTOM_LEFT,
    TAngle::TOP_LEFT | TAngle::BOTTOM_LEFT,
    TAngle::TOP_LEFT,
    TAngle::TOP_LEFT | TAngle::TOP_RIGHT,
    TAngle::TOP_RIGHT,
    TAngle::TOP_RIGHT | TAngle::BOTTOM_RIGHT,
    TAngle::BOTTOM_RIGHT,
};

namespace NGeoHash {
    namespace NTile {
        THolder<geos::geom::Polygon> BoundingBoxToPolygon(const NGeoHash::TBoundingBoxLL& bbox) {
            auto gsf = geos::util::GeometricShapeFactory(geos::geom::GeometryFactory::getDefaultInstance());
            gsf.setNumPoints(4);
            gsf.setBase(geos::geom::Coordinate(bbox.GetMinX(), bbox.GetMinY()));
            gsf.setWidth(bbox.Width());
            gsf.setHeight(bbox.Height());
            std::unique_ptr<geos::geom::Polygon> polygonHolder(gsf.createRectangle());
            return THolder<geos::geom::Polygon>(polygonHolder.release());
        }

        THolder<geos::geom::Polygon> CircleToPolygon(const NGeo::TPointLL& center, double radius, ui8 nPoints) {
            auto latRadius = NGeo::GetLongitudeFromMetersAtEquator(radius);
            auto lonRadius = NGeo::GetWidthAtLatitude(latRadius, center);

            auto gsf = geos::util::GeometricShapeFactory(geos::geom::GeometryFactory::getDefaultInstance());
            gsf.setCentre(geos::geom::Coordinate(center.Lon(), center.Lat()));
            gsf.setNumPoints(nPoints);
            gsf.setWidth(lonRadius * 2);
            gsf.setHeight(latRadius * 2);
            std::unique_ptr<geos::geom::Polygon> polygonHolder(gsf.createCircle());
            return THolder<geos::geom::Polygon>(polygonHolder.release());
        }

        TVector<TGeoHashDescriptor> TileGeometryWithDescriptors(const geos::geom::Geometry& geometry, ui8 steps,
                                                                double minimumIntersectionRatio) {
            minimumIntersectionRatio = ClampVal(minimumIntersectionRatio, 0.0, 1.0);
            TVector<TGeoHashDescriptor> result;
            const geos::geom::Envelope& geometryBBox = *geometry.getEnvelopeInternal();

            // Start from bottom-left geohash and go up and right until there's no intersections
            TGeoHashDescriptor latIterator(geometryBBox.getMinY(), geometryBBox.getMinX(), steps); // Y - lat, X - lon

            while (true) {
                auto geoHashBBox = BoundingBoxToPolygon(latIterator.ToBoundingBox());
                if (!geometryBBox.intersects(geoHashBBox->getEnvelopeInternal())) {
                    break;
                }
                auto lonIterator = latIterator;
                while (true) {
                    auto geoHashBBox = BoundingBoxToPolygon(lonIterator.ToBoundingBox());
                    if (!geometryBBox.intersects(geoHashBBox->getEnvelopeInternal())) {
                        break;
                    }

                    double intersectionArea;
                    try {
                        auto intersectionHolder = geometry.intersection(geoHashBBox.Get());
                        THolder<geos::geom::Geometry> intersection(intersectionHolder.release());
                        intersectionArea = intersection->getArea();
                    } catch (geos::util::TopologyException& _) {
                        intersectionArea = 0.0;
                    }

                    if (intersectionArea > 0.0 && intersectionArea / geoHashBBox->getArea() >= minimumIntersectionRatio) {
                        result.push_back(lonIterator);
                    }
                    lonIterator = lonIterator.GetNeighbour(EDirection::EAST).GetRef();
                }
                auto latNext = latIterator.GetNeighbour(EDirection::NORTH);
                if (latNext.Empty()) {
                    break;
                }
                latIterator = latNext.GetRef();
            }
            return result;
        }

        TVector<TString> TileGeometryWithStrings(const geos::geom::Geometry& geometry, ui8 precision,
                                                 double minimumIntersectionRatio) {
            auto descriptors = TileGeometryWithDescriptors(geometry, TGeoHashDescriptor::PrecisionToSteps(precision),
                                                           minimumIntersectionRatio);
            TVector<TString> result(Reserve(descriptors.size()));
            for (auto geohashDescriptor : descriptors) {
                result.push_back(geohashDescriptor.ToString());
            }
            return result;
        }

        TVector<ui64> TileGeometryWithBits(const geos::geom::Geometry& geometry, ui8 precision,
                                           double minimumIntersectionRatio) {
            auto descriptors = TileGeometryWithDescriptors(geometry, TGeoHashDescriptor::PrecisionToSteps(precision),
                                                           minimumIntersectionRatio);
            TVector<ui64> result(Reserve(descriptors.size()));
            for (auto geohashDescriptor : descriptors) {
                result.push_back(geohashDescriptor.GetBits());
            }
            return result;
        }

        TVector<TGeoHashDescriptor>
        TileGeometryWithDescriptorsExternal(const geos::geom::Geometry& geometry, ui8 steps) {
            return TileGeometryWithDescriptors(geometry, steps, 0.0);
        }

        TVector<TGeoHashDescriptor>
        TileGeometryWithDescriptorsInternal(const geos::geom::Geometry& geometry, ui8 steps) {
            return TileGeometryWithDescriptors(geometry, steps, 1.0);
        }

        TVector<TString> TileGeometryWithStringsExternal(const geos::geom::Geometry& geometry, ui8 precision) {
            return TileGeometryWithStrings(geometry, precision, 0.0);
        }

        TVector<TString> TileGeometryWithStringsInternal(const geos::geom::Geometry& geometry, ui8 precision) {
            return TileGeometryWithStrings(geometry, precision, 1.0);
        }

        TVector<ui64> TileGeometryWithBitsExternal(const geos::geom::Geometry& geometry, ui8 precision) {
            return TileGeometryWithBits(geometry, precision, 0.0);
        }

        TVector<ui64> TileGeometryWithBitsInternal(const geos::geom::Geometry& geometry, ui8 precision) {
            return TileGeometryWithBits(geometry, precision, 1.0);
        }

        TVector<TString>
        CoverCircle(double lat, double lon, double radius, ui8 precision, ui8 nPoints) {
            return TileGeometryWithStringsExternal(
                *(CircleToPolygon({lon, lat}, radius, nPoints)),
                precision);
        }

        double ApproximateSquaredDistance(double deltaLat, double deltaLon, double latCos) {
            double deltaY = NGeo::GetMetersFromDeg(deltaLat);
            double deltaX = NGeo::GetMetersFromDeg(deltaLon) * latCos;
            return deltaX * deltaX + deltaY * deltaY;
        }

        TVector<NGeo::TPointLL>
        GetAngles(TBoundingBoxLL bBox) {
            return {
                {bBox.GetMinX(), bBox.GetMinY()},
                {bBox.GetMaxX(), bBox.GetMinY()},
                {bBox.GetMinX(), bBox.GetMaxY()},
                {bBox.GetMaxX(), bBox.GetMaxY()}};
        };

        TVector<TGeoHashDescriptor>
        CoverCircleWithDesciptorsFast(double lat, double lon, double radius, ui8 steps) {
            using Vertex = std::pair<TGeoHashDescriptor, EDirection>;

            TSet<ui64> watched;
            double radiusSquared = radius * radius;
            double centerLatCos = NGeo::GetLatCos(lat);
            TGeoHashDescriptor centerGeoHash = {lat, lon, steps};
            TVector<TGeoHashDescriptor> result = {centerGeoHash};

            // First, add 1st level neighbours and remember their directions
            TDeque<Vertex> verticesToWatch;

            auto centerNeighbours = centerGeoHash.GetNeighbours();
            for (auto direction : GetEnumAllValues<NGeoHash::EDirection>()) {
                if (centerNeighbours[direction].Defined()) {
                    verticesToWatch.emplace_back(centerNeighbours[direction].GetRef(), direction);
                    watched.insert(centerNeighbours[direction]->GetBits());
                }
            }

            // Then, do BFS for geohashes, checking them for distance
            Vertex currentVertex;

            while (verticesToWatch) {
                currentVertex = verticesToWatch.front();
                bool accepted = false;
                auto geoHash = currentVertex.first;
                auto geoHashBBox = geoHash.ToBoundingBox();
                auto originalDirection = currentVertex.second;
                auto angleMask = angleMasks[originalDirection];
                auto bBoxAngles = GetAngles(geoHashBBox);

                for (auto i : xrange(4)) {
                    if (angleMask & angles[i]) {
                        auto sqDist = ApproximateSquaredDistance(
                            bBoxAngles[i].Lat() - lat, bBoxAngles[i].Lon() - lon, centerLatCos);
                        if (sqDist <= radiusSquared) {
                            accepted = true;
                            break;
                        }
                    }
                }

                if (accepted) {
                    result.push_back(geoHash);
                    for (auto neighbour : geoHash.GetNeighbours()) {
                        if (neighbour.Defined() && !watched.contains(neighbour->GetBits())) {
                            watched.insert(neighbour->GetBits());
                            verticesToWatch.emplace_back(neighbour.GetRef(), originalDirection);
                        }
                    }
                }
                verticesToWatch.pop_front();
            }

            return result;
        }

        TVector<TString>
        CoverCircleFast(double lat, double lon, double radius, ui8 precision) {
            auto descriptors = CoverCircleWithDesciptorsFast(
                lat, lon, radius, TGeoHashDescriptor::PrecisionToSteps(precision));
            TVector<TString> result(Reserve(descriptors.size()));
            for (auto geoHashDescriptor : descriptors) {
                result.push_back(geoHashDescriptor.ToString());
            }
            return result;
        }
    }
}
