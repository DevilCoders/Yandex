#include <library/cpp/reverse_geocoder/core/reverse_geocoder.h>
#include <library/cpp/reverse_geocoder/core/geo_data/debug.h>
#include <library/cpp/reverse_geocoder/core/geo_data/map.h>
#include <library/cpp/reverse_geocoder/generator/generator.h>
#include <library/cpp/reverse_geocoder/generator/mut_geo_data.h>
#include <library/cpp/reverse_geocoder/library/pool_allocator.h>
#include <library/cpp/reverse_geocoder/library/stop_watch.h>
#include <library/cpp/reverse_geocoder/library/log.h>
#include <library/cpp/reverse_geocoder/test/unittest/unittest.h>
#include <library/cpp/testing/unittest/registar.h>
#include <utility>

using namespace NReverseGeocoder;

template <typename valT>
static bool IsEqual(valT const& a, valT const& b) {
    return !memcmp(&a, &b, sizeof(valT));
}

template <typename TArr>
static bool IsEqual(const TArr* a, const TArr* b, TNumber number) {
    for (TNumber i = 0; i < number; ++i)
        if (!IsEqual(a[i], b[i]))
            return false;
    return true;
}

static bool operator==(const IGeoData& a, const IGeoData& b) {
#define GEO_BASE_DEF_VAR(TVar, Var) \
    if (a.Var() != b.Var())         \
        return false;

#define GEO_BASE_DEF_ARR(TArr, Arr)                  \
    if (a.Arr##Number() != b.Arr##Number())          \
        return false;                                \
    if (!IsEqual(a.Arr(), b.Arr(), a.Arr##Number())) \
        return false;

    GEO_BASE_DEF_GEO_DATA

#undef GEO_BASE_DEF_VAR
#undef GEO_BASE_DEF_ARR

    return true;
}

Y_UNIT_TEST_SUITE(TGeoDataMap) {
    Y_UNIT_TEST(SimpleSerialize) {
        TPoolAllocator allocator(MB);

        NGenerator::TMutGeoData geoData;
        geoData.SetVersion(GEO_DATA_CURRENT_VERSION);

        TGeoDataMap geoDataMap1(geoData, &allocator);
        TGeoDataMap geoDataMap2(geoDataMap1.Data(), geoDataMap1.Size());

        UNIT_ASSERT(geoDataMap1 == geoData);
        UNIT_ASSERT(geoDataMap1 == geoDataMap2);
    }

    Y_UNIT_TEST(CheckVersion) {
        TPoolAllocator allocator(MB);

        NGenerator::TMutGeoData geoData;
        geoData.SetVersion(GEO_DATA_VERSION_0);

        UNIT_ASSERT_EXCEPTION(TGeoDataMap(geoData, &allocator), yexception);
    }

    Y_UNIT_TEST(FakeDataSerialize) {
        TPoolAllocator allocator(64 * MB);
        TPoolAllocator serializeAllocator(64 * MB);

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

        TGeoDataMap geoDataMap1(geoData, &serializeAllocator);
        TGeoDataMap geoDataMap2(geoDataMap1.Data(), geoDataMap1.Size());

        UNIT_ASSERT(geoDataMap1 == geoData);
        UNIT_ASSERT(geoDataMap1 == geoDataMap2);

        TReverseGeocoder geoBase(geoDataMap2);

        UNIT_ASSERT_EQUAL(123ull, geoBase.Lookup(TLocation(5, 5)));
        UNIT_ASSERT_EQUAL(UNKNOWN_GEO_ID, geoBase.Lookup(TLocation(-1, -1)));
        UNIT_ASSERT_EQUAL(UNKNOWN_GEO_ID, geoBase.Lookup(TLocation(0, 3)));
        UNIT_ASSERT_EQUAL(123ull, geoBase.Lookup(TLocation(4, 0)));
        UNIT_ASSERT_EQUAL(123ull, geoBase.Lookup(TLocation(5, 5)));

        NGeoData::Show(Cout, geoDataMap2);
    }
}
