#include "convert.h"

#include <library/cpp/reverse_geocoder/proto_library/writer.h>
#include <library/cpp/reverse_geocoder/proto/region.pb.h>

#include <geos/geom/Geometry.h>
#include <geos/geom/MultiPolygon.h>
#include <geos/geom/Point.h>
#include <geos/geom/Polygon.h>
#include <geos/io/WKTReader.h>

#include <util/generic/xrange.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/split.h>

namespace NReverseGeocoder {
    namespace {
        void ParseLinearRing(NProto::TPolygon& polygon, const geos::geom::LinearRing* ring) {
            for (auto i : xrange(ring->getNumPoints())) {
                auto location = polygon.AddLocations();
                location->SetLon(ring->getPointN(i)->getX());
                location->SetLat(ring->getPointN(i)->getY());
            }
        }

        void ParsePolygon(NProto::TRegion& region, const geos::geom::Polygon* polygon) {
            ui64 id = 1;
            auto protoPoly = region.AddPolygons();
            protoPoly->SetPolygonId(id++);
            protoPoly->SetType(NProto::TPolygon::TYPE_OUTER);
            ParseLinearRing(*protoPoly, polygon->getExteriorRing());

            for (auto i : xrange(polygon->getNumInteriorRing())) {
                auto protoPoly = region.AddPolygons();
                protoPoly->SetPolygonId(id++);
                protoPoly->SetType(NProto::TPolygon::TYPE_INNER);
                ParseLinearRing(*protoPoly, polygon->getInteriorRingN(i));
            }
        }

        void ParseMultiPolygon(NProto::TRegion& region, const geos::geom::MultiPolygon* multiPolygon) {
            for (auto i : xrange(multiPolygon->getNumGeometries())) {
                auto polygon = static_cast<const geos::geom::Polygon*>(multiPolygon->getGeometryN(i));
                ParsePolygon(region, polygon);
            }
        }

        void ParseWktGeometry(NProto::TRegion& region, const geos::geom::Geometry* geometry) {
            switch (geometry->getGeometryTypeId()) {
                case geos::geom::GEOS_POLYGON:
                    ParsePolygon(region, static_cast<const geos::geom::Polygon*>(geometry));
                    break;
                case geos::geom::GEOS_MULTIPOLYGON:
                    ParseMultiPolygon(region, static_cast<const geos::geom::MultiPolygon*>(geometry));
                    break;
                default:
                    ythrow yexception() << "Unexpected geometry type: " << geometry->getGeometryType();
            }
        }
    }

    void ConvertTsv(const TString& inputPath, const TString& outputPath) {
        TFileInput input(inputPath);
        NProto::TWriter writer(outputPath.c_str());
        geos::io::WKTReader wktReader;

        TString line;
        while (input.ReadLine(line) != 0) {
            NProto::TRegion region;
            ui64 regionId;
            TString wkt;

            // First column is region id, second column is WKT geometry, rest of columns are debug info
            StringSplitter(line).Split('\t').Take(2).CollectInto(&regionId, &wkt);
            std::unique_ptr<geos::geom::Geometry> geometry = wktReader.read(wkt);

            region.SetRegionId(regionId);
            ParseWktGeometry(region, geometry.get());
            writer.Write(region);
        }
    }
}
