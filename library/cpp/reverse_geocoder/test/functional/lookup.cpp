#include <library/cpp/reverse_geocoder/core/reverse_geocoder.h>
#include <library/cpp/reverse_geocoder/core/geo_data/debug.h>
#include <library/cpp/reverse_geocoder/generator/generator.h>
#include <library/cpp/reverse_geocoder/generator/gen_geo_data.h>
#include <library/cpp/reverse_geocoder/generator/mut_geo_data.h>
#include <library/cpp/reverse_geocoder/library/memory.h>
#include <library/cpp/reverse_geocoder/open_street_map/converter.h>
#include <library/cpp/reverse_geocoder/test/unittest/unittest.h>

#include <utility>
#include <sstream>
#include <iomanip>

using namespace NReverseGeocoder;

Y_UNIT_TEST_SUITE(TLookup) {
    Y_UNIT_TEST(RawLookupTest) {
        NGenerator::TConfig config;
        config.SaveRawBorders = true;

        NGenerator::TMutGeoData geoData;
        NGenerator::TGenerator generator(config, &geoData);

        NProto::TRegion region;

        NProto::TPolygon* polygon = region.AddPolygons();
        polygon->SetPolygonId(123);

        using TPair = std::pair<int, int>;
        using TVectorType = TVector<TPair>;

        for (const TPair& p : TVectorType{{0, 0}, {10, 0}, {10, 0}, {10, 10}}) {
            NProto::TLocation* l = polygon->AddLocations();
            l->SetLon(p.first);
            l->SetLat(p.second);
        }

        region.SetRegionId(123);

        generator.Init();
        generator.Update(region);
        generator.Fini();

        TReverseGeocoder reverseGeocoder(geoData);

        UNIT_ASSERT_EQUAL(123ull, reverseGeocoder.RawLookup(TLocation(5, 5)));
        UNIT_ASSERT_EQUAL(UNKNOWN_GEO_ID, reverseGeocoder.RawLookup(TLocation(-1, -1)));
        UNIT_ASSERT_EQUAL(UNKNOWN_GEO_ID, reverseGeocoder.RawLookup(TLocation(0, 3)));
        UNIT_ASSERT_EQUAL(123ull, reverseGeocoder.RawLookup(TLocation(4, 0)));
        UNIT_ASSERT_EQUAL(123ull, reverseGeocoder.RawLookup(TLocation(5, 5)));
    }

    std::string ToString(TVector<TGeoId> const& a) {
        std::stringstream out;
        out << "[";
        for (size_t i = 0; i < a.size(); ++i) {
            if (i > 0)
                out << ", ";
            out << a[i];
        }
        out << "]";
        return out.str();
    }

    std::string ToString(const TLocation& l) {
        std::stringstream out;
        out << std::fixed << std::setprecision(6);
        out << "(" << l.Lat << ", " << l.Lon << ")";
        return out.str();
    }

    Y_UNIT_TEST(b2b) {
        UNIT_ASSERT_NO_EXCEPTION(NOpenStreetMap::RunPoolConvert("andorra-latest.osm.pbf", "andorra-latest.pbf", 1));

        NGenerator::TConfig config;
        {
            config.InputPath = "andorra-latest.pbf";
            config.OutputPath = "andorra-latest.dat";
            config.SaveRawBorders = true;
        };
        UNIT_ASSERT_NO_EXCEPTION(NGenerator::Generate(config));

        TReverseGeocoder reverseGeocoder("andorra-latest.dat");

        const IGeoData& geoData = reverseGeocoder.GeoData();

        static size_t const X_GRID = 1000;
        static size_t const Y_GRID = 1000;

        TReverseGeocoder::TDebug lookupDebug;
        TReverseGeocoder::TDebug rawLookupDebug;

        const TBoundingBox box = TBoundingBox(geoData.Points(), geoData.PointsNumber());

        for (size_t x = 0; x < X_GRID; ++x) {
            for (size_t y = 0; y < Y_GRID; ++y) {
                TLocation location;

                location.Lon = ToDouble(box.X1);
                location.Lat = ToDouble(box.Y1);

                location.Lon += ToDouble(box.X2 - box.X1) * x / X_GRID;
                location.Lat += ToDouble(box.Y2 - box.Y1) * y / Y_GRID;

                const TGeoId lookupId = reverseGeocoder.Lookup(location, &lookupDebug);
                const TGeoId rawLookupId = reverseGeocoder.RawLookup(location, &rawLookupDebug);

                UNIT_ASSERT_EQUAL(rawLookupId, lookupId);
                UNIT_ASSERT_EQUAL(rawLookupDebug.size(), lookupDebug.size());

                if (rawLookupDebug.size() == lookupDebug.size())
                    for (size_t i = 0; i < rawLookupDebug.size(); ++i)
                        UNIT_ASSERT_EQUAL(rawLookupDebug[i], lookupDebug[i]);
            }
        }
    }
}
