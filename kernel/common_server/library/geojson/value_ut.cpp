#include "value.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(GeoJsonSuite) {
    Y_UNIT_TEST(Serialization) {
        NGeoJson::TFeatureCollection featureCollection;
        featureCollection.GetMetadata().emplace("funny_number", 42);

        NGeoJson::TFeature feature;
        feature.SetId(43);

        NGeoJson::TGeometry geometry(NGeoJson::TGeometry::Polygon);
        geometry.GetCoordinates().emplace_back(37, 55);
        geometry.GetCoordinates().emplace_back(42, 42);
        feature.SetGeometry(geometry);
        UNIT_ASSERT(feature.GetGeometry());

        featureCollection.GetFeatures().push_back(feature);

        auto serialized = NJson::ToJson(featureCollection);
        UNIT_ASSERT(serialized.IsDefined());
        NGeoJson::TFeatureCollection deserialized;
        UNIT_ASSERT(NJson::TryFromJson(serialized, deserialized));

        UNIT_ASSERT_VALUES_EQUAL(deserialized.GetMetadata().at("funny_number").GetUInteger(), 42);
        UNIT_ASSERT_VALUES_EQUAL(deserialized.GetFeatures().size(), 1);

        const auto& dfeature = deserialized.GetFeatures()[0];
        UNIT_ASSERT_VALUES_EQUAL(dfeature.GetId(), feature.GetId());
        UNIT_ASSERT(dfeature.GetGeometry());
        UNIT_ASSERT_VALUES_EQUAL(dfeature.GetGeometry()->GetType(), NGeoJson::TGeometry::Polygon);
        UNIT_ASSERT_VALUES_EQUAL(dfeature.GetGeometry()->GetCoordinates().size(), 2);
    }
}
