#include "utils.h"

#include <library/cpp/protobuf/from_xml/map_field.h>
#include <library/cpp/protobuf/from_xml/ut/proto/basic.pb.h>
#include <library/cpp/testing/unittest/registar.h>

#include <contrib/libs/pugixml/pugixml.hpp>

using namespace NProtobufFromXml;

namespace {
    const XmlDocPtr XML_DOCUMENT = LoadDocument(R"(
        <root>
            <repeated>
                <value>1</value>
                <value>2</value>
                <value>42</value>
            </repeated>
            <required value="24"/>
            <optional>true</optional>
        </root>
    )");
}

Y_UNIT_TEST_SUITE(MapFieldTests) {
    Y_UNIT_TEST(MapField_forRepeatedField_addsAllValuesToIt) {
        TestMessage msg;
        TPbFieldWrapper field{msg, "repeated_field"};
        MapField(
            field,
            XML_DOCUMENT->select_nodes("//repeated/value"),
            TPrimitiveFieldMapping<ui64>{});
        UNIT_ASSERT_EQUAL(msg.repeated_field_size(), 3);
    }

    Y_UNIT_TEST(MapField_forOptionalFieldAndSingleNode_setsValue) {
        TestMessage msg;
        TPbFieldWrapper field{msg, "optional_field"};
        MapField(
            field,
            XML_DOCUMENT->select_nodes("//optional"),
            TPrimitiveFieldMapping<bool>{});
        UNIT_ASSERT(msg.has_optional_field());
    }

    Y_UNIT_TEST(MapField_forOptionalFieldAndMultipleNodes_throws) {
        TestMessage msg;
        TPbFieldWrapper field{msg, "optional_field"};
        UNIT_ASSERT_EXCEPTION(
            MapField(
                field,
                XML_DOCUMENT->select_nodes("//repeated/value"),
                TPrimitiveFieldMapping<bool>{}),
            yexception);
    }

    Y_UNIT_TEST(MapField_forRequiredFieldAndNoNodes_throws) {
        TestMessage msg;
        TPbFieldWrapper field{msg, "required_field"};
        UNIT_ASSERT_EXCEPTION(
            MapField(
                field,
                XML_DOCUMENT->select_nodes("//nonexistent"),
                TPrimitiveFieldMapping<ui64>{}),
            yexception);
    }

    Y_UNIT_TEST(MapField_forRequiredFieldAndMultipleNodes_throws) {
        TestMessage msg;
        TPbFieldWrapper field{msg, "required_field"};
        UNIT_ASSERT_EXCEPTION(
            MapField(
                field,
                XML_DOCUMENT->select_nodes("//repeated/value"),
                TPrimitiveFieldMapping<ui64>{}),
            yexception);
    }

    Y_UNIT_TEST(MapField_forRequiredFieldAndSingleNode_setsValue) {
        TestMessage msg;
        TPbFieldWrapper field{msg, "required_field"};
        MapField(
            field,
            XML_DOCUMENT->select_nodes("//required/@value"),
            TPrimitiveFieldMapping<ui32>{});
        UNIT_ASSERT(msg.has_required_field());
    }
}
