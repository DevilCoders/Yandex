#include "tile.h"

#include <geos/geom/Geometry.h>
#include <geos/io/WKTReader.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <util/stream/file.h>

using namespace NGeoHash::NTile;

Y_UNIT_TEST_SUITE(GEOSGeometryTileByGeoHashes) {
    Y_UNIT_TEST(BoundingBoxToPolygon_) {
        auto g = BoundingBoxToPolygon({{37.0, 55.0},
                                       {38.0, 56.0}});
        UNIT_ASSERT(g->isRectangle());
        UNIT_ASSERT_DOUBLES_EQUAL(g->getArea(), 1.0, 0.01);
    }

    Y_UNIT_TEST(TileGeometry_) {
        auto g = BoundingBoxToPolygon({{37.0, 55.0},
                                       {38.0, 56.0}});
        constexpr size_t precision = 5;
        auto internalGeoHashes = TileGeometryWithBitsInternal(*g, precision);
        auto externalGeoHashes = TileGeometryWithBitsExternal(*g, precision);
        UNIT_ASSERT(internalGeoHashes.size() < externalGeoHashes.size());
    }

    Y_UNIT_TEST(TileGeometryShouldnotThrow) {

        geos::io::WKTReader wktReader;
        TIFStream geometries(GetWorkPath() + "/unusual_geometries.txt");
        TString geometryLine;

        while (geometries.ReadLine(geometryLine)) {
            auto geometry = wktReader.read(geometryLine);
            UNIT_ASSERT_NO_EXCEPTION(TileGeometryWithStringsExternal(*geometry, 8));
        }
    }
}
