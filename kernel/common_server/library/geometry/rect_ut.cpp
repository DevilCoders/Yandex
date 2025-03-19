#include "rect.h"

#include <kernel/common_server/library/json/cast.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(GeoRectSuite) {
    Y_UNIT_TEST(Serialization) {
        auto original = TGeoRect(37, 55, 38, 56);
        {
            auto serialized = ToString(original);
            auto deserialized = FromString<TGeoRect>(serialized);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Min.X, deserialized.Min.X, 0.00001);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Min.Y, deserialized.Min.Y, 0.00001);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Max.X, deserialized.Max.X, 0.00001);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Max.Y, deserialized.Max.Y, 0.00001);
        }
        {
            auto serialized = ToString(original);
            TGeoRect deserialized;
            UNIT_ASSERT(TryFromString(serialized, deserialized));
            UNIT_ASSERT_DOUBLES_EQUAL(original.Min.X, deserialized.Min.X, 0.00001);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Min.Y, deserialized.Min.Y, 0.00001);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Max.X, deserialized.Max.X, 0.00001);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Max.Y, deserialized.Max.Y, 0.00001);
        }
        {
            auto serialized = NJson::ToJson(original);
            auto deserialized = NJson::FromJson<TGeoRect>(serialized);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Min.X, deserialized.Min.X, 0.00001);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Min.Y, deserialized.Min.Y, 0.00001);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Max.X, deserialized.Max.X, 0.00001);
            UNIT_ASSERT_DOUBLES_EQUAL(original.Max.Y, deserialized.Max.Y, 0.00001);
        }
    }
}
