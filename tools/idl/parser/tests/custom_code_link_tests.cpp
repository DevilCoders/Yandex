#include "test_helpers.h"

#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/parser/parse_idl.h>
#include <yandex/maps/idl/utils/errors.h>

#include <memory>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {
namespace custom_code_tests {

class Iterator : public nodes::StrictVisitor {
public:
    void onVisited(const nodes::Enum& e) override
    {
        BOOST_CHECK_EQUAL(e.name.original(), "E");
        BOOST_REQUIRE(e.customCodeLink.baseHeader);
        BOOST_CHECK_EQUAL(*e.customCodeLink.baseHeader, "some_header.h");
        BOOST_CHECK(!e.customCodeLink.protoconvHeader);
    }
    void onVisited(const nodes::Struct& s) override
    {
        BOOST_CHECK_EQUAL(s.name.original(), "S");
        BOOST_REQUIRE(s.customCodeLink.baseHeader);
        BOOST_CHECK_EQUAL(*s.customCodeLink.baseHeader,
            "some/other_header.h");
        BOOST_REQUIRE(s.customCodeLink.protoconvHeader);
        BOOST_CHECK_EQUAL(*s.customCodeLink.protoconvHeader,
            "proto_conv_header.h");
    }
    void onVisited(const nodes::Variant& v) override
    {
        BOOST_CHECK_EQUAL(v.name.original(), "V");
        BOOST_CHECK(!v.customCodeLink.baseHeader);
        BOOST_REQUIRE(v.customCodeLink.protoconvHeader);
        BOOST_CHECK_EQUAL(*v.customCodeLink.protoconvHeader,
            "proto_header.h");
    }
};

BOOST_AUTO_TEST_CASE(custom_code_link)
{
    BOOST_CHECK_THROW(parseIdl("", "cpp struct S{}"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "protoconv struct S{}"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "cpp protoconv struct S{}"), utils::GroupedError);

    const std::string VALID_TEXT =
        "cpp \"some_header.h\"\n"
        "enum E based on \"a/b.proto\":c.d {"
        "    A"
        "}"
        "cpp \"some/other_header.h\"\n"
        "protoconv \"proto_conv_header.h\"\n"
        "struct S {"
        "}"
        ""
        "protoconv \"proto_header.h\"\n"
        "variant V{"
        "    int i;"
        "    float f;"
        "}";
    const auto root = parseIdl("", VALID_TEXT);
    BOOST_CHECK_EQUAL(root.nodes.count(), 3);

    root.nodes.traverse(Iterator());
}

} // namespace custom_code_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
