#include "test_helpers.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/parser/parse_idl.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/utils/errors.h>
#include <yandex/maps/idl/utils/exception.h>

#include <memory>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {
namespace general_tests {

BOOST_AUTO_TEST_CASE(empty_idl)
{
    BOOST_CHECK_THROW(parseIdl("", ""), utils::GroupedError);
}

BOOST_AUTO_TEST_CASE(valid_files)
{
    auto env = simpleEnv("parser/tests/idl_frameworks", "parser/tests/idl_root", false);

    env.idl("test/test.idl");
    env.idl("sample/sample.idl");

    auto idl = env.idl("test/test.idl");
    BOOST_CHECK(idl->root.objcInfix.empty());

    auto idl2 = env.idl("sample/sample.idl");
    BOOST_CHECK_EQUAL(idl2->root.objcInfix, "Sample");
}

BOOST_AUTO_TEST_CASE(multi_root_files)
{
    auto env = simpleEnv("parser/tests/idl_frameworks", "parser/tests/idl_root", false);
    BOOST_CHECK_THROW(env.idl("simple/simple.idl"), utils::Exception);

    auto env2 = simpleEnv("parser/tests/idl_frameworks",
        { "parser/tests/idl_root", "parser/tests/another_idl_root" }, false);
    env2.idl("simple/simple.idl");
}

BOOST_AUTO_TEST_CASE(imports)
{
    BOOST_CHECK_THROW(parseIdl("", "import \"ya::ru\";"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "import \"ya/ru/file.idl\""), utils::GroupedError);
    parseIdl("", "import \"ya/ru/file.idl\";");
    BOOST_CHECK_THROW(parseIdl("", "import ya/ru/file.idl;"), utils::GroupedError);
    BOOST_CHECK_THROW(
        parseIdl("", "import \"ya/ru/file.idl\";import \"yandex.idl\""),
        utils::GroupedError);
    parseIdl("", "import \"ya/ru/file.idl\";import \"yandex/ru/file.idl\";");
    parseIdl("", "\n\nimport \"ya/ru/file.idl\";\nimport \"yandex.idl\";\n");
}

BOOST_AUTO_TEST_CASE(scope_tests)
{
    BOOST_CHECK_EQUAL(
        Scope({ "one", "two", "three" }).asString("+"), "one+two+three");
    BOOST_CHECK_EQUAL(
        Scope({ "one", "two", "three" }).asPrefix("+"), "one+two+three+");

    auto env = simpleEnv("parser/tests/idl_frameworks", "parser/tests/another_idl_root", false);

    auto idl = env.idl("another/another_simple.idl");
    BOOST_CHECK_EQUAL(idl->cppNamespace + "cpp_one" + "ten",
        Scope({ "yandex", "maps", "mapkit", "search", "cpp_one", "ten" }));
    BOOST_CHECK_EQUAL(idl->objcTypePrefix + "one" + "objc_ten",
        Scope({ "YMK", "Search", "one", "objc_ten" }));
    BOOST_CHECK_EQUAL(idl->javaPackage,
        Scope({ "ru", "yandex", "maps", "mapkit", "search"}));

    BOOST_CHECK_EQUAL(idl->idlNamespace + "one" + "two",
        Scope({ "another", "one", "two" }));
    BOOST_CHECK_EQUAL(idl->idlNamespace + Scope::Items({ "three", "four" }),
        Scope({ "another", "three", "four" }));
    BOOST_CHECK_EQUAL(idl->idlNamespace, Scope("another"));
}

} // namespace general_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
