#include "utils.h"

#include <library/cpp/protobuf/from_xml/node_text.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NProtobufFromXml;

namespace {
    const XmlDocPtr XML_DOCUMENT = LoadDocument(R"(
    <test>
        <node1 value="42"/>
        <node2>this is a test</node2>
        <node3>this <p>is</p> a <highlight>te</highlight>st</node3>
    </test>
    )");
}

Y_UNIT_TEST_SUITE(NodeTextTests) {
    Y_UNIT_TEST(NodeText_givenAttributeXpathNode_returnsAttributeValue) {
        UNIT_ASSERT_EQUAL(
            NodeText(XML_DOCUMENT->select_node("//node1/@value")),
            "42");
    }

    Y_UNIT_TEST(NodeText_givenSimpleTextNode_returnsNodeText) {
        UNIT_ASSERT_EQUAL(
            NodeText(XML_DOCUMENT->select_node("//node2")),
            "this is a test");
    }

    Y_UNIT_TEST(NodeText_givenComplexTextNode_returnsFullText) {
        UNIT_ASSERT_EQUAL(
            NodeText(XML_DOCUMENT->select_node("//node3")),
            "this is a test");
    }
}
