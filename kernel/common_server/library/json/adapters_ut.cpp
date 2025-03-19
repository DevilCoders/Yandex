#include "adapters.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <array>

Y_UNIT_TEST_SUITE(JsonCastAdaptersSuite) {
    Y_UNIT_TEST(Dictionary) {
        TMap<TString, i64> value = {
            { "fake", 42 },
            { "news", -42 },
        };

        auto serialized = NJson::ToJson(NJson::Dictionary(value));
        UNIT_ASSERT(serialized.IsMap());
        const auto& m = serialized.GetMapSafe();
        UNIT_ASSERT_VALUES_EQUAL(m.at("fake").GetIntegerSafe(), 42);
        UNIT_ASSERT_VALUES_EQUAL(m.at("news").GetIntegerSafe(), -42);

        TMap<TString, i64> deserialized;
        UNIT_ASSERT(NJson::TryFromJson(serialized, NJson::Dictionary(deserialized)));
        UNIT_ASSERT_VALUES_EQUAL(value.size(), deserialized.size());
        UNIT_ASSERT_VALUES_EQUAL(value.at("fake"), deserialized.at("fake"));
        UNIT_ASSERT_VALUES_EQUAL(value.at("news"), deserialized.at("news"));
    }

    Y_UNIT_TEST(HexString) {
        std::array<ui16, 3> original;
        original[0] = 42;
        original[1] = 3;
        original[2] = 81;
        auto serialized = NJson::ToJson(NJson::HexString(original));
        UNIT_ASSERT(serialized.IsString());

        std::array<ui16, 3> restored;
        UNIT_ASSERT(NJson::TryFromJson(serialized, NJson::HexString(restored)));
        UNIT_ASSERT_VALUES_EQUAL(original[0], restored[0]);
        UNIT_ASSERT_VALUES_EQUAL(original[1], restored[1]);
        UNIT_ASSERT_VALUES_EQUAL(original[2], restored[2]);
    }

    Y_UNIT_TEST(JsonString) {
        TString dictString = R"(
{"first": "value", "second": 42}
        )";

        auto dict = NJson::ToJson(NJson::JsonString(dictString));
        UNIT_ASSERT(dict.IsMap());
        UNIT_ASSERT(dict.GetMapSafe().contains("first"));
        UNIT_ASSERT(dict.GetMapSafe().contains("second"));

        TString arrayString = R"(
["fake", 42]
        )";

        auto arr = NJson::ToJson(NJson::JsonString(arrayString));
        UNIT_ASSERT(arr.IsArray());
        UNIT_ASSERT_VALUES_EQUAL(arr.GetArraySafe().size(), 2);
    }

    Y_UNIT_TEST(Nullable) {
        NJson::TJsonValue dict;
        TString first;
        TString second;
        dict["first"] = NJson::ToJson(first);
        dict["second"] = NJson::ToJson(NJson::Nullable(second));

        auto serialized = dict.GetStringRobust();
        Cerr << serialized << Endl;
        auto deserialized = NJson::ReadJsonFastTree(serialized);

        UNIT_ASSERT(deserialized["first"].IsString());
        UNIT_ASSERT(deserialized["second"].IsNull());
    }

    Y_UNIT_TEST(Stringify) {
        TFsPath original = GetOutputPath();
        auto serialized = NJson::ToJson(NJson::Stringify(original));
        UNIT_ASSERT(serialized.IsString());

        TFsPath deserialized;
        UNIT_ASSERT(NJson::TryFromJson(serialized, NJson::Stringify(deserialized)));
        UNIT_ASSERT_VALUES_EQUAL(original, deserialized);
    }
}
