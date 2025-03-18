#include "test_helpers.h"

#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/parser/parse_idl.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/utils/errors.h>

#include <memory>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {
namespace variant_tests {

class IdlNodesIterator : public nodes::StrictVisitor {
public:
    void onVisited(const nodes::Variant& v) override
    {
        if (v.name.original() == "V1") {
            BOOST_CHECK(!v.protoMessage);
            BOOST_CHECK_EQUAL(v.fields.size(), 1);
            BOOST_CHECK(v.fields[0].typeRef.id == nodes::TypeId::Float);
            BOOST_REQUIRE(!v.fields[0].protoField);
        } else if (v.name.original() == "V2") {
            BOOST_CHECK(!v.protoMessage);
            BOOST_CHECK_EQUAL(v.fields.size(), 6);
            BOOST_CHECK_EQUAL(v.fields[0].name, "i");
            BOOST_CHECK(v.fields[1].typeRef.id == nodes::TypeId::Double);
            BOOST_REQUIRE(!v.fields[1].protoField);
            BOOST_CHECK_EQUAL(v.fields[2].typeRef.parameters.size(), 1);
            BOOST_CHECK_EQUAL(*v.fields[2].typeRef.parameters[0].name,
                Scope({ "n", "s", "V" }));
            BOOST_CHECK_EQUAL(v.fields[3].name, "ms");
            BOOST_CHECK(v.fields[3].typeRef.id == nodes::TypeId::Dictionary);
            BOOST_CHECK_EQUAL(v.fields[3].typeRef.parameters.size(), 2);
            BOOST_CHECK_EQUAL(*v.fields[3].typeRef.parameters[1].name,
                Scope({ "some", "value", "V" }));
            BOOST_CHECK(v.fields[4].typeRef.id == nodes::TypeId::Custom);
            BOOST_REQUIRE(!v.fields[4].protoField);
            BOOST_CHECK_EQUAL(*v.fields[5].typeRef.name, Scope("S"));
        } else {
            BOOST_FAIL("wrong variant name!");
        }
    }
};

BOOST_AUTO_TEST_CASE(variants)
{
    BOOST_CHECK_THROW(parseIdl("", "variant V"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "variant V { }"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "variant V { struct S { } }"),
        utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "variant V { variant V { } }"),
        utils::GroupedError);

    const std::string VALID_TEXT =
        "variant V1 {"
        "    float f;"
        "}"
        "variant V2 {"
        "    int i;"
        "    double d;"
        "    vector<n.s.V> vs;"
        "    dictionary<some.key.K, some.value.V> ms;"
        "    E e;"
        "    S s;"
        "}";
    const auto root = parseIdl("", VALID_TEXT);
    BOOST_CHECK_EQUAL(root.nodes.count(), 2);

    root.nodes.traverse(IdlNodesIterator());
}

} // namespace variant_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
