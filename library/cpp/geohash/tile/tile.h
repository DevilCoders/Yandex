#pragma once

#include <library/cpp/geohash/geohash.h>

#include <util/generic/vector.h>

#include <contrib/libs/geos/include/geos/geom/Geometry.h>
#include <contrib/libs/geos/include/geos/geom/Polygon.h>

namespace NGeoHash {
    namespace NTile {
        /**
         * Tile GEOS Geometry with TGeoHashDescriptor's
         * @param steps TGeoHashDesciptor step, available values: 0..62
         * @param minimumIntersectionRatio For particular geohash cell: minimal ratio between area of intersection with geometry
         * and cell's area to be added in result vector
         */
        TVector<TGeoHashDescriptor>
        TileGeometryWithDescriptors(const geos::geom::Geometry& geometry, ui8 steps,
                                    double minimumIntersectionRatio);

        // TileGeometryWithDescriptors with minimumIntersectionShare == 0.0 -> Any intersection
        TVector<TGeoHashDescriptor>
        TileGeometryWithDescriptorsExternal(const geos::geom::Geometry& geometry, ui8 steps);

        // TileGeometryWithDescriptors with minimumIntersectionShare == 1.0 -> Only internal geohash cells
        TVector<TGeoHashDescriptor>
        TileGeometryWithDescriptorsInternal(const geos::geom::Geometry& geometry, ui8 steps);

        // Same but tiling with TString
        TVector<TString>
        TileGeometryWithStrings(const geos::geom::Geometry& geometry, ui8 precision,
                                double minimumIntersectionShare);

        TVector<TString>
        TileGeometryWithStringsExternal(const geos::geom::Geometry& geometry, ui8 precision);

        TVector<TString>
        TileGeometryWithStringsInternal(const geos::geom::Geometry& geometry, ui8 precision);

        // Same but tiling with geohash bits (ui64)
        TVector<ui64>
        TileGeometryWithBits(const geos::geom::Geometry& geometry, ui8 precision,
                             double minimumIntersectionShare);

        TVector<ui64>
        TileGeometryWithBitsExternal(const geos::geom::Geometry& geometry, ui8 precision);

        TVector<ui64>
        TileGeometryWithBitsInternal(const geos::geom::Geometry& geometry, ui8 precision);

        THolder<geos::geom::Polygon>
        BoundingBoxToPolygon(const NGeoHash::TBoundingBoxLL& bbox);

        THolder<geos::geom::Polygon>
        CircleToPolygon(const NGeo::TPointLL& point, double radius, ui8 nPoints = 32);

        TVector<TString>
        CoverCircle(double lat, double lon, double radius, ui8 precision, ui8 nPoints = 32);

        /* Faster version of previous function with some heuristics */
        TVector<TString>
        CoverCircleFast(double lat, double lon, double radius, ui8 precision);
    }
}
