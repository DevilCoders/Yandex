#include <library/cpp/reverse_geocoder/core/reverse_geocoder.h>
#include <library/cpp/reverse_geocoder/core/geo_data/debug.h>
#include <library/cpp/reverse_geocoder/generator/generator.h>
#include <library/cpp/reverse_geocoder/generator/gen_geo_data.h>
#include <library/cpp/reverse_geocoder/generator/mut_geo_data.h>
#include <library/cpp/reverse_geocoder/library/memory.h>
#include <library/cpp/reverse_geocoder/open_street_map/converter.h>
#include <library/cpp/reverse_geocoder/test/unittest/unittest.h>
#include <util/system/filemap.h>
#include <utility>

using namespace NReverseGeocoder;

Y_UNIT_TEST_SUITE(TGenerator) {
    Y_UNIT_TEST(GeneratorTest) {
        NGenerator::TMutGeoData geoData;
        NGenerator::TGenerator generator(NGenerator::TConfig(), &geoData);

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

        UNIT_ASSERT_EQUAL(123ull, reverseGeocoder.Lookup(TLocation(5, 5)));
        UNIT_ASSERT_EQUAL(UNKNOWN_GEO_ID, reverseGeocoder.Lookup(TLocation(-1, -1)));
        UNIT_ASSERT_EQUAL(UNKNOWN_GEO_ID, reverseGeocoder.Lookup(TLocation(0, 3)));
        UNIT_ASSERT_EQUAL(123ull, reverseGeocoder.Lookup(TLocation(4, 0)));
        UNIT_ASSERT_EQUAL(123ull, reverseGeocoder.Lookup(TLocation(5, 5)));
    }

    Y_UNIT_TEST(Dump) {
        UNIT_ASSERT_NO_EXCEPTION(NOpenStreetMap::RunPoolConvert("andorra-latest.osm.pbf", "andorra-latest.pbf", 1));

        NGenerator::TConfig config;
        {
            config.InputPath = "andorra-latest.pbf";
            config.OutputPath = "andorra-latest.dat.test";
            config.SaveRawBorders = true;
        };
        UNIT_ASSERT_NO_EXCEPTION(NGenerator::Generate(config));

        TFileMap pre("andorra-latest.dat.pre");
        pre.Map(0, pre.Length());

        TFileMap test("andorra-latest.dat.test");
        test.Map(0, test.Length());

        UNIT_ASSERT_UNEQUAL(0u, pre.MappedSize());
        UNIT_ASSERT_UNEQUAL(0u, test.MappedSize());

        TGeoDataMap map1((const char*)pre.Ptr(), pre.MappedSize());
        TGeoDataMap map2((const char*)test.Ptr(), test.MappedSize());

        NGeoData::Show(Cout, map1);
        NGeoData::Show(Cout, map2);

        TReverseGeocoder reverseGeocoder((const char*)test.Ptr(), test.MappedSize());

        UNIT_ASSERT_EQUAL(pre.MappedSize(), test.MappedSize());
        UNIT_ASSERT(NGeoData::Equals(map1, map2));
        UNIT_ASSERT(NGeoData::Equals(map2, reverseGeocoder.GeoData()));
    }
}
