#include <kernel/geodb/fixlist.h>
#include <kernel/geodb/geodb.h>

#include <kernel/search_types/search_types.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/stream/file.h>

const static TString FILE_NAME("geodb.bin");

static void BestGeoTest(const TVector<TCateg>& ids, TCateg userRegionId, TCateg expectedBestGeoId, const NGeoDB::TGeoKeeper& geoDb) {
    UNIT_ASSERT_VALUES_EQUAL(GetBestGeo(userRegionId, TSet<TCateg>(ids.begin(), ids.end()), geoDb)->GetId(), expectedBestGeoId);

    const auto userRegionPtr = geoDb.Find(userRegionId);
    const auto expectedBestGeoPtr = geoDb.Find(expectedBestGeoId);
    TSet<NGeoDB::TGeoPtr> ptrSet;
    for (const TCateg id : ids)
        ptrSet.insert(geoDb.Find(id));
    UNIT_ASSERT_VALUES_EQUAL(GetBestGeo(userRegionPtr, ptrSet), expectedBestGeoPtr);
}

static void ShortestDistanceTest(const TCateg idA, const TCateg idB, const size_t distance, const NGeoDB::TGeoKeeper& geoDb) {
    const auto geoA = geoDb.Find(idA);
    const auto geoB = geoDb.Find(idB);
    UNIT_ASSERT_VALUES_EQUAL(GetShortestDistance(geoA, geoB), distance);
}

Y_UNIT_TEST_SUITE(TKernelGeoDBShortestDistanceTests) {
    Y_UNIT_TEST(TestShortestDistance) {
        const auto geo = NGeoDB::TGeoKeeper::LoadToHolder(FILE_NAME);
        const auto& geoDb = *geo.Get();
        ShortestDistanceTest(213, 213, 0, geoDb);
        ShortestDistanceTest(213, 10001, 4, geoDb);

        ShortestDistanceTest(2, 193, 5, geoDb);
        ShortestDistanceTest(2, 29357, 6, geoDb);
        ShortestDistanceTest(146, 10430, 7, geoDb);

    }
}

Y_UNIT_TEST_SUITE(TKernelGeoDBBestGeoTests) {
    Y_UNIT_TEST(TestBestGeo) {
        TFileInput in(FILE_NAME);
        const auto geo = NGeoDB::TGeoKeeper::LoadToHolder(in);
        const auto& geoDb = *geo.Get();

        // empty set
        // 2 = Petersburg
        BestGeoTest({}, END_CATEG, END_CATEG, geoDb);
        BestGeoTest({}, 2, END_CATEG, geoDb);

        // no user region
        BestGeoTest({2}, END_CATEG, 2, geoDb);

        // single id
        // 213 = Moscow
        BestGeoTest({2}, 213, 2, geoDb);
        BestGeoTest({2}, 2, 2, geoDb);
        BestGeoTest({2}, END_CATEG, 2, geoDb);

        // GetBestMinWeight
        // 116271 = airport Pulkovo
        // 200 = Los Angeles
        BestGeoTest({2, 116271}, 2, 2, geoDb);
        BestGeoTest({2, 116271}, 200, 2, geoDb);
        BestGeoTest({2, 116271}, 116271, 2, geoDb);
        BestGeoTest({2, 116271}, END_CATEG, 2, geoDb);

        // GetBestGeoSameRegion
        BestGeoTest({2, 116271, 213}, 2, 2, geoDb);
        BestGeoTest({2, 116271, 213}, 213, 213, geoDb);
        BestGeoTest({10747, 2, 213}, 10747, 10747, geoDb);

        // GetBestGeoSameConstituent
        // 10884 = Pushkin
        BestGeoTest({2, 116271, 213}, 10884, 2, geoDb);

        // GetBestGeoCapital
        BestGeoTest({2, 116271, 213}, 200, 213, geoDb);
        BestGeoTest({2, 116271, 213}, END_CATEG, 213, geoDb);

        // GetBestGeoMainInConstituentEntity
        // 194 = Saratov
        // 240 = Tol'yatti
        // 101397 and 100540 = small cities from Moldova
        BestGeoTest({194, 240}, 200, 194, geoDb);
        BestGeoTest({194, 240}, 2, 194, geoDb);
        BestGeoTest({194, 240}, 213, 194, geoDb);
        BestGeoTest({101397, 100540}, 213, 101397, geoDb);

        // GetBestGeoLongestPath
        // 51 = Samara
        // 24 = Velyki Novgorod
        BestGeoTest({240, 10884}, 24, 10884, geoDb);
        // 105655 = Estado Libre y Soberano de Durango in Mexico
        // 29372 = Markham from canada
        BestGeoTest({240, 29372}, 200, 29372, geoDb);
        BestGeoTest({240, 29372}, 100540, 240, geoDb);

        // GetBestGeoRUBK
        // 28896 = Artemivsk from Ukraine
        // 105655 = Estado Libre y Soberano de Durango in Mexico
        // 105790 = Ararat from Armenia
        // 157 = Minsk
        BestGeoTest({28896, 105790, 105655}, 157, 28896, geoDb);
        // user is not from RUBK
        BestGeoTest({28896, 105790, 105655}, END_CATEG, 105655, geoDb);

        // GetBestGeoBiggestEntry
        // 29372 = Province of Ontario
        // 202 = New York
        BestGeoTest({202, 29372}, 28896, 29372, geoDb);
        BestGeoTest({29372, 202}, 213, 29372, geoDb);
        BestGeoTest({29372, 202}, END_CATEG, 29372, geoDb);

        // GetBestGeoMainInFederalDistrict
        // 10502 = Paris
        // 10393 = London
        BestGeoTest({10502, 10393}, 2, 10393, geoDb);
        BestGeoTest({10502, 10393}, 200, 10393, geoDb);
        BestGeoTest({10502, 10393}, 213, 10393, geoDb);
        BestGeoTest({10502, 10393}, END_CATEG, 10393, geoDb);

        // min ID
        // 10425 = Copenhagen
        // 177 = Berlin
        //BestGeoTest({10472, 177}, 2, 177, geoDb);
        BestGeoTest({177, 10425}, 2, 177, geoDb);
        BestGeoTest({10425, 177}, 200, 177, geoDb);
        BestGeoTest({10425, 177}, 213, 177, geoDb);
        BestGeoTest({10425, 177}, END_CATEG, 177, geoDb);

        //bad id
        BestGeoTest({2, -1}, 2, 2, geoDb);
        BestGeoTest({28896, -1, 105790, 105655, -1}, 157, 28896, geoDb);
        BestGeoTest({-1, 202, 10000000, 29372, -1000}, 28896, 29372, geoDb);

        // FixBest
        // 10430 = Valencia
        // 204 = Spain
        BestGeoTest({10015,10021,10108,10134,10430,20785,21057,21187,29343,102811,105687,109288,109381,110679,114082,114118,114195,114784,122097},
                     204, 10430, geoDb);
        // 21185 = Caracas
        BestGeoTest({10015,10021,10108,10134,10430,20785,21057,21187,29343,102811,105687,109288,109381,110679,114082,114118,114195,114784,122097},
                     21185, 114784, geoDb);

        // Fix Better
        // 10129 = Philadelphia (USA, PA)
        BestGeoTest({202,10013,10129,21131,21379,29321,29330,29331,29348,29350,29359,103751,104218,104520,123613},
                    2, 10129, geoDb);
        // Fix Better
        // 10129 = Philadelphia (USA, PA)
        BestGeoTest({202,10013,10129,21131,21191,21379,29321,29330,29331,29348,29350,29359,103751,123613,145166,145384},
                    2, 10129, geoDb);
    }
}

Y_UNIT_TEST_SUITE(TKernelGeoDBBestGeoFixList) {
    Y_UNIT_TEST(TestValid) {
        const auto geo = NGeoDB::TGeoKeeper::LoadToHolder(FILE_NAME);

        for (const auto& fix : NGeoDB::NDetail::FIX_BEST) {
            UNIT_ASSERT(END_CATEG != geo->Find(fix.From)->GetId());
            UNIT_ASSERT(END_CATEG != geo->Find(fix.Best)->GetId());
        }
        for (const auto& fix : NGeoDB::NDetail::FIX_BETTER) {
            UNIT_ASSERT(END_CATEG != geo->Find(fix.From)->GetId());
            UNIT_ASSERT(END_CATEG != geo->Find(fix.Better)->GetId());
            UNIT_ASSERT(END_CATEG != geo->Find(fix.Worse)->GetId());
        }
    }
}
