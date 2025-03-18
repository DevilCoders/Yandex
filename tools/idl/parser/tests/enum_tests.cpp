#include "test_helpers.h"

#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/parser/parse_idl.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/utils/errors.h>

#include <memory>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {
namespace enum_tests {

class Iterator : public nodes::StrictVisitor {
public:
    void onVisited(const nodes::Enum& e) override
    {
        if (e.name.original() == "E1") {
            BOOST_REQUIRE(e.protoMessage);
            BOOST_CHECK_EQUAL(e.protoMessage->pathToProto, "a/b.proto");
            BOOST_CHECK_EQUAL(e.protoMessage->pathInProto, Scope({ "c", "d" }));
            BOOST_CHECK_EQUAL(e.fields.size(), 1);
            BOOST_CHECK_EQUAL(e.fields[0].name, "A");
            BOOST_CHECK(!e.fields[0].value);
        } else if (e.name.original() == "E2") {
            BOOST_CHECK(!e.protoMessage);
            BOOST_CHECK_EQUAL(e.fields.size(), 5);
            BOOST_CHECK_EQUAL(e.fields[0].name, "A");
            BOOST_CHECK_EQUAL(*e.fields[0].value, "-1");
            BOOST_REQUIRE(e.fields[1].value);
            BOOST_CHECK_EQUAL(*e.fields[1].value, "A");
            BOOST_CHECK_EQUAL(e.fields[2].name, "C");
            BOOST_REQUIRE(e.fields[2].value);
            BOOST_CHECK_EQUAL(*e.fields[2].value, "CONST");
            BOOST_CHECK_EQUAL(e.fields[3].name, "D");
            BOOST_REQUIRE(e.fields[4].value);
            BOOST_CHECK_EQUAL(*e.fields[4].value,
                "C * A + 0x1 - 2.3f / (0.004 + 567L)");
        } else {
            BOOST_FAIL("wrong enum name!");
        }
    }
};

BOOST_AUTO_TEST_CASE(namespace_with_enums)
{
    BOOST_CHECK_THROW(parseIdl("", "enum E"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "enum E{}"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "enum E{A,}"), utils::GroupedError);

    const std::string VALID_TEXT =
        "/**\n"
        " * ... some documentation ...\n"
        " */\n"
        "bitfield enum E1 based on \"a/b.proto\":c.d {\n"
        "    A\n"
        "}\n"
        "enum E2 {\n"
        "    A=-1,\n"
        "    B=A,\n"
        "    C = CONST,\n"
        "    D = C << 1,\n"
        "    E = C * A + 0x1 - 2.3f / (0.004 + 567L)\n"
        "}";
    const auto root = parseIdl("", VALID_TEXT);
    BOOST_CHECK_EQUAL(root.nodes.count(), 2);

    root.nodes.traverse(Iterator());
}

} // namespace enum_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
