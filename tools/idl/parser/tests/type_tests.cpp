#include "test_helpers.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/parser/parse_idl.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/targets.h>

#include <memory>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {
namespace type_tests {

BOOST_AUTO_TEST_CASE(types)
{
    auto env = simpleEnv("parser/tests/idl_frameworks", "parser/tests/idl_root", false);

    auto idl = env.idl("type_test1/type_test1.idl");

    Scope where;
    auto what = Scope({ "type_test2", "Address2" });
    const auto& typeInfo1 = idl->type(where, what);
    BOOST_CHECK_EQUAL(
        typeInfo1.idl->relativePath, "type_test2/type_test2.idl");
    BOOST_CHECK_EQUAL(
        std::get<const nodes::Struct*>(typeInfo1.type)->name.original(),
        "Address2");

    where += "Address1";
    const auto& typeInfo2 = idl->type(where, Scope("Component1"));
    BOOST_CHECK_EQUAL(
        typeInfo2.idl->relativePath, "type_test1/type_test1.idl");
    BOOST_CHECK_EQUAL(
        std::get<const nodes::Struct*>(typeInfo2.type)->name.original(),
        "Component1");

    where += "Component1";
    what = Scope("Address1");
    const auto& typeInfo3 = idl->type(where, what);
    BOOST_CHECK_EQUAL(typeInfo3.name.original(), "Address1");
    BOOST_CHECK_EQUAL(typeInfo3.idl->idlNamespace, Scope("type_test1"));
    BOOST_CHECK_EQUAL(typeInfo3.scope.original(),
        Scope({ "Address1", "Component1" }));
    BOOST_CHECK(std::get<const nodes::Struct*>(typeInfo3.type)->kind ==
        nodes::StructKind::Options);

    BOOST_CHECK_EQUAL(typeInfo3.fullNameAsScope[OBJC].asString("::"),
        "YMK::Component1::Address1");
    BOOST_CHECK_EQUAL(typeInfo3.fullNameAsScope.original().asString("."),
        "type_test1.Address1.Component1.Address1");
    BOOST_CHECK_EQUAL(
        typeInfo3.scope[CS].asString("."), "Address1.Component1");
}

} // namespace type_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
