#include "test_helpers.h"

#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/parser/parse_idl.h>
#include <yandex/maps/idl/utils/errors.h>

#include <memory>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {
namespace name_tests {

class Iterator : public nodes::StrictVisitor {
public:
    void onVisited(const nodes::Enum& e) override
    {
        BOOST_CHECK_EQUAL(e.name.original(), "E");
        BOOST_CHECK_EQUAL(e.name["cs"], "ECs");
        BOOST_CHECK_EQUAL(e.name["java"], "EJava");
        BOOST_CHECK_EQUAL(e.name["some unsupported target"], "E");
    }
    void onVisited(const nodes::Variant& v) override
    {
        BOOST_CHECK_EQUAL(v.name.original(), "V");
        BOOST_CHECK_EQUAL(v.name["java"], "name");
    }
};

BOOST_AUTO_TEST_CASE(platform_names)
{
    BOOST_CHECK_THROW(parseIdl("", "interface I (II:) { }"),
        utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "struct S (:SS) { }"),
        utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "struct S (cs:S S) { }"),
        utils::GroupedError);

    const auto root = parseIdl("",
        "enum E (cs:ECs, java:EJava) based on \"a/b.proto\":c.d.e {"
        "    C"
        "}"
        "variant V (java:name) {"
        "    int i;"
        "}");
    BOOST_CHECK_EQUAL(root.nodes.count(), 2);

    root.nodes.traverse(Iterator());
}

} // namespace name_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
