#include "builder.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(JsonBuilderSuite) {
    Y_UNIT_TEST(Map) {
        NJson::TJsonValue result = NJson::TMapBuilder("fake", "me")("value", 42);
        UNIT_ASSERT_VALUES_EQUAL(result["fake"].GetString(), "me");
        UNIT_ASSERT_VALUES_EQUAL(result["value"].GetUInteger(), 42);
    }

    Y_UNIT_TEST(Array) {
        NJson::TJsonValue result = NJson::TArrayBuilder("42")(42);
        UNIT_ASSERT(result.IsArray());
        UNIT_ASSERT_VALUES_EQUAL(result.GetArray().size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(result.GetArray().at(0).GetString(), "42");
        UNIT_ASSERT_VALUES_EQUAL(result.GetArray().at(1).GetInteger(), 42);
    }
}
