#include <kernel/geodb/geodb.h>
#include <kernel/geodb/util.h>
#include <kernel/geodb/entity.h>

#include <kernel/search_types/search_types.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/stream/file.h>

const static TString FILE_NAME("geodb.bin");

Y_UNIT_TEST_SUITE(TKernelGeoDBTests) {
    Y_UNIT_TEST(TestKeeperLoad) {
        TFileInput in(FILE_NAME);
        NGeoDB::TGeoKeeper geo;
        geo.Load(&in);
        UNIT_ASSERT_VALUES_EQUAL(geo.Find(213)->GetId(), 213);
    }

    Y_UNIT_TEST(TestKeeperLoadToHolder) {
        {
            TFileInput in(FILE_NAME);
            NGeoDB::TGeoKeeperHolder geo = NGeoDB::TGeoKeeper::LoadToHolder(in);
            UNIT_ASSERT_VALUES_EQUAL(geo->Find(213)->GetId(), 213);
        }
        {
            const auto geodb = NGeoDB::TGeoKeeper::LoadToHolder(FILE_NAME);
            UNIT_ASSERT_VALUES_EQUAL(geodb->Find(213)->GetId(), 213);
        }
    }

    Y_UNIT_TEST(TestGetXXX) {
        TFileInput in(FILE_NAME);
        NGeoDB::TGeoKeeperHolder geo = NGeoDB::TGeoKeeper::LoadToHolder(in);

        NGeoDB::TGeoPtr moscow = geo->Find(213);
        UNIT_ASSERT(moscow);
        UNIT_ASSERT(moscow == geo->Find(213));
        UNIT_ASSERT_VALUES_EQUAL(moscow->GetId(), 213);
        UNIT_ASSERT_VALUES_EQUAL(moscow.Parent()->GetId(), 1);

        UNIT_ASSERT_VALUES_EQUAL(moscow.Country()->GetId(), 225);
        UNIT_ASSERT(moscow.Country() == geo->Find(225));
        UNIT_ASSERT(moscow.Country().CountryCapital() == moscow);
        UNIT_ASSERT(geo->Find(225).Capital() == moscow);
        UNIT_ASSERT(!moscow.Capital());
        UNIT_ASSERT(geo->Find(225).Capital());

        NGeoDB::TGeoPtr mosObl = geo->Find(1);
        UNIT_ASSERT(mosObl);
        UNIT_ASSERT_VALUES_EQUAL(moscow.Parent(), mosObl);
        UNIT_ASSERT_VALUES_EQUAL(moscow->GetParentId(), mosObl->GetId());
        UNIT_ASSERT(moscow != mosObl);

        // XXX add more tests for invalid region
        UNIT_ASSERT(!geo->Find(-1));
        UNIT_ASSERT(!geo->Find(-1).Parent());
        UNIT_ASSERT_VALUES_EQUAL(geo->Find(-1), geo->Find(-100));
        UNIT_ASSERT_VALUES_EQUAL(geo->Find(-100)->GetId(), -1); // FIXME END_CATEG

        UNIT_ASSERT_VALUES_EQUAL(moscow.ParentByType(NGeoDB::COUNTRY)->GetId(), 225);

        UNIT_ASSERT(moscow.IsIn(mosObl));

        NGeoDB::TGeoPtr turky = geo->Find(983);
        UNIT_ASSERT(turky);
        UNIT_ASSERT(!moscow.IsIn(turky));

        UNIT_ASSERT(moscow->HasPopulation());
        UNIT_ASSERT(moscow->GetPopulation() > 10000000);

        UNIT_ASSERT_VALUES_EQUAL(moscow->PhoneCodeSize(), 2);
        TString mskCode = moscow->GetPhoneCode(0);
        mskCode.append(moscow->GetPhoneCode(1));
        UNIT_ASSERT(mskCode == "495499" || mskCode == "499495");
        UNIT_ASSERT_VALUES_EQUAL(geo->Find(225)->PhoneCodeSize(), 1);
    }

    Y_UNIT_TEST(TestGetNames) {
        TFileInput in(FILE_NAME);
        NGeoDB::TGeoKeeperHolder geo = NGeoDB::TGeoKeeper::LoadToHolder(in);

        const NGeoDB::TRegionProto::TNames* names = geo->Find(213).FindNames(LANG_RUS);

        UNIT_ASSERT(names);
        UNIT_ASSERT_VALUES_EQUAL(names->GetNominative(), "Москва");
        UNIT_ASSERT_VALUES_EQUAL(names->GetPreposition(), "в");

        names = geo->Find(213).FindNames("ru");
        UNIT_ASSERT(names);
        UNIT_ASSERT_VALUES_EQUAL(names->GetNominative(), "Москва");

        names = geo->Find(-1).FindNames("ru");
        UNIT_ASSERT(!names);
    }

    Y_UNIT_TEST(TestInvalidFileLoadFromProtobuf) {
        const TString fileContent("hello there");
        TStringInput fn(fileContent);
        UNIT_ASSERT_EXCEPTION(NGeoDB::TGeoKeeper::LoadToHolder(fn), yexception);
    }

    Y_UNIT_TEST(TestEmtpy) {
        {
            const NGeoDB::TGeoKeeper geodb;
            UNIT_ASSERT(geodb.Empty());

            UNIT_ASSERT_VALUES_EQUAL(SpanAsString(geodb.Find(1)), "0,0");
            UNIT_ASSERT_VALUES_EQUAL(LocationAsString(geodb.Find(1)), "0,0");
        }
        {
            TFileInput input{FILE_NAME};
            const auto geodb = NGeoDB::TGeoKeeper::LoadToHolder(input);
            UNIT_ASSERT(!geodb->Empty());
        }
    }

    Y_UNIT_TEST(Disambiguate) {
        NGeoDB::TGeoKeeperHolder db = NGeoDB::TGeoKeeper::LoadToHolder(FILE_NAME);

        struct SData {
            TCateg Region;
            TCateg UserRegion;
            TVector<TCateg> DisambList;
        };
        // TODO add more examples
        const TVector<SData> regList = {
            { 10872, 213, {225, 10174} }, // kirovs in ru (spb)
            { 24887, 187, {} }, // kirovsk in ua
            { 102765, 225, {84} }, // melburne in usa from ru
            { 102765, 84, {} } // melburne in usa from usa
        };

        for (const SData& reg : regList) {
            NGeoDB::TGeoPtr region = db->Find(reg.Region);
            UNIT_ASSERT(region);
            NGeoDB::TGeoPtr userRegion = db->Find(reg.UserRegion);
            UNIT_ASSERT(userRegion);

            TVector<TCateg> disambList;
            UNIT_ASSERT_VALUES_EQUAL(region.Disambiguate(userRegion, disambList), (bool)reg.DisambList);
            if (reg.DisambList)
                UNIT_ASSERT_VALUES_EQUAL(reg.DisambList, disambList);
        }
    }

    Y_UNIT_TEST(EntityTypeWeight) {
        const NGeoDB::TEntityTypeWeight CEAWeight = 7;
        UNIT_ASSERT_EXCEPTION(NGeoDB::EntityTypeToWeight(static_cast<NGeoDB::EType>(NGeoDB::EType_MAX + 1)), yexception);
        UNIT_ASSERT_EXCEPTION(NGeoDB::EntityTypeToWeight(NGeoDB::EType::UNKNOWN), yexception);
        UNIT_ASSERT_VALUES_EQUAL(NGeoDB::EntityTypeToWeight(NGeoDB::EType::CONSTITUENT_ENTITY_AREA), CEAWeight);

        NGeoDB::TEntityTypeWeight weight = 120;
        UNIT_ASSERT(!NGeoDB::TryEntityTypeToWeight(static_cast<NGeoDB::EType>(NGeoDB::EType_MAX + 1), weight));
        UNIT_ASSERT_VALUES_EQUAL(weight, 120);

        UNIT_ASSERT(!NGeoDB::TryEntityTypeToWeight(NGeoDB::EType::UNKNOWN, weight));
        UNIT_ASSERT_VALUES_EQUAL(weight, 120);

        UNIT_ASSERT(NGeoDB::TryEntityTypeToWeight(NGeoDB::EType::CONSTITUENT_ENTITY_AREA, weight));
        UNIT_ASSERT_VALUES_EQUAL(weight, CEAWeight);
    }

    Y_UNIT_TEST(Util) {
        const auto geodb = NGeoDB::TGeoKeeper::LoadToHolder(FILE_NAME);
        NGeoDB::TGeoPtr msk = geodb->Find(213);

        UNIT_ASSERT_VALUES_EQUAL(SpanAsString(msk), "0.641442,0.466439");
        UNIT_ASSERT_VALUES_EQUAL(LocationAsString(msk), "37.620393,55.75396");
        UNIT_ASSERT_VALUES_EQUAL(SpanAsString(msk, false), "0.641442,0.466439");
        UNIT_ASSERT_VALUES_EQUAL(LocationAsString(msk, false), "37.620393,55.75396");
        UNIT_ASSERT_VALUES_EQUAL(SpanAsString(msk, true), "0.466439,0.641442");
        UNIT_ASSERT_VALUES_EQUAL(LocationAsString(msk, true), "55.75396,37.620393");

        UNIT_ASSERT(IsKUBR(geodb->Find(213)));
        UNIT_ASSERT(IsKUBR(geodb->Find(143)));
        UNIT_ASSERT(IsKUBR(geodb->Find(157)));
        UNIT_ASSERT(IsKUBR(geodb->Find(163)));
        UNIT_ASSERT(!IsKUBR(geodb->Find(10511)));
        UNIT_ASSERT(IsKUBR(geodb->Find(225)));
        UNIT_ASSERT(!IsKUBR(geodb->Find(983)));
    }
}
