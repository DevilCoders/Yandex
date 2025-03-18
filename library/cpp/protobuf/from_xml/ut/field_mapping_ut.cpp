#include <library/cpp/protobuf/from_xml/field_mapping.h>
#include <library/cpp/protobuf/from_xml/pb_field_wrapper.h>
#include <library/cpp/protobuf/from_xml/ut/proto/basic.pb.h>
#include <library/cpp/testing/unittest/registar.h>

#include <contrib/libs/pugixml/pugixml.hpp>

#include <util/generic/string.h>

#include <cstdint>

using namespace NProtobufFromXml;

Y_UNIT_TEST_SUITE(PrimitiveFieldMappingTests) {
    Y_UNIT_TEST(Set_whenMapFuncReturnsValue_setsField) {
        const ui32 value = 42;
        TPrimitiveFieldMapping<ui32> mapping{
            [=](const TString&) { return value; }};
        TestMessage msg;
        TPbFieldWrapper field{msg, "required_field"};
        mapping.Set(field, pugi::xpath_node{});

        UNIT_ASSERT(msg.has_required_field());
        UNIT_ASSERT_EQUAL(msg.required_field(), value);
    }

    Y_UNIT_TEST(Set_whenMapFuncReturnsNothing_doesntSetField) {
        TPrimitiveFieldMapping<ui32> mapping{
            [](const TString&) { return TMaybe<ui32>{}; }};
        TestMessage msg;
        TPbFieldWrapper field{msg, "required_field"};
        mapping.Set(field, pugi::xpath_node{});

        UNIT_ASSERT(!msg.has_required_field());
    }

    Y_UNIT_TEST(Add_whenMapFuncReturnsValue_addsValue) {
        const ui64 value = 42;
        TPrimitiveFieldMapping<ui64> mapping{
            [=](const TString&) { return value; }};
        TestMessage msg;
        TPbFieldWrapper field{msg, "repeated_field"};
        mapping.Add(field, pugi::xpath_node{});

        UNIT_ASSERT_EQUAL(msg.repeated_field_size(), 1);
        UNIT_ASSERT_EQUAL(msg.repeated_field(0), value);
    }

    Y_UNIT_TEST(Add_whenMapFuncReturnsNothing_doesntAddValue) {
        TPrimitiveFieldMapping<ui64> mapping{
            [](const TString&) { return TMaybe<ui64>{}; }};
        TestMessage msg;
        TPbFieldWrapper field{msg, "repeated_field"};
        mapping.Add(field, pugi::xpath_node{});

        UNIT_ASSERT_EQUAL(msg.repeated_field_size(), 0);
    }
}
