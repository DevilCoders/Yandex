#include "cast.h"
#include "parse.h"

#include <kernel/common_server/library/geometry/coord.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

#include <tuple>

Y_UNIT_TEST_SUITE(JsonCastSuite) {
    Y_UNIT_TEST(Containers) {
        TString jsonString = R"(
[
    "fake",
    "news"
]
        )";
        NJson::TJsonValue json = NJson::ReadJsonFastTree(jsonString);

        auto v = NJson::FromJson<TVector<TString>>(json);
        UNIT_ASSERT_VALUES_EQUAL(v.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(v[0], "fake");
        UNIT_ASSERT_VALUES_EQUAL(v[1], "news");

        auto s = NJson::FromJson<TSet<TString>>(json);
        UNIT_ASSERT_VALUES_EQUAL(s.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(*s.begin(), "fake");
    }

    Y_UNIT_TEST(Dicts) {
        TMap<ui32, TString> dict = {
            { 0, "fake" },
            { 42, "news" }
        };

        auto serialized = NJson::ToJson(dict);
        Cout << serialized.GetStringRobust() << Endl;

        auto deserialized = NJson::FromJson<THashMultiMap<double, TStringBuf>>(serialized);
        UNIT_ASSERT_VALUES_EQUAL(deserialized.size(), 2);
    }

    Y_UNIT_TEST(Tuples) {
        {
            auto value = std::make_pair(42, TStringBuf("fakenews"));
            auto serialized = NJson::ToJson(value);
            Cout << serialized.GetStringRobust() << Endl;

            auto deserialized = NJson::FromJson<std::pair<double, TString>>(serialized);
            UNIT_ASSERT_DOUBLES_EQUAL(deserialized.first, 42, 0.001);
            UNIT_ASSERT_VALUES_EQUAL(deserialized.second, "fakenews");
        }
        {
            auto value = std::make_tuple(42, TStringBuf("fakenews"), 1.234);
            auto serialized = NJson::ToJson(value);
            Cout << serialized.GetStringRobust() << Endl;

            auto deserialized = NJson::FromJson<std::tuple<double, TString, float>>(serialized);
            UNIT_ASSERT_DOUBLES_EQUAL(std::get<0>(deserialized), 42, 0.001);
            UNIT_ASSERT_VALUES_EQUAL(std::get<1>(deserialized), "fakenews");
            UNIT_ASSERT_DOUBLES_EQUAL(std::get<2>(deserialized), 1.234, 0.001);
        }
    }

    Y_UNIT_TEST(GeoCoord) {
        auto str = TString("[[37.329277736025276,55.62958866520693],[37.34112237103504,55.637114585080994]]");
        auto serialized = NJson::ReadJsonFastTree(str);
        TVector<TGeoCoord> coordinates;
        UNIT_ASSERT(NJson::ParseField(serialized, coordinates));
        UNIT_ASSERT_VALUES_EQUAL(coordinates.size(), 2);
    }
}
