#include "test_helpers.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/parser/parse_framework.h>
#include <yandex/maps/idl/utils/exception.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {
namespace framework_parsing_tests {

BOOST_AUTO_TEST_CASE(correct_framework_parsing)
{
    auto env = simpleEnv("parser/tests/idl_frameworks", "parser/tests/idl_root", false);

    auto idl = env.idl("test_framework/test.idl");
    BOOST_CHECK_EQUAL(idl->cppNamespace,
        Scope({ "yandex", "maps", "idl", "test" }));
    BOOST_CHECK_EQUAL(idl->javaPackage,
        Scope({ "ru", "yandex", "maps", "idl", "test" }));
    BOOST_CHECK_EQUAL(idl->objcTypePrefix, Scope({ "YMIP"}));
    BOOST_CHECK_EQUAL(idl->objcFramework, "YandexMapsIdlParser");
}

BOOST_AUTO_TEST_CASE(incorrect_framework_parsing)
{
    auto env = simpleEnv("parser/tests/idl_frameworks", "parser/tests/idl_root", false);
    BOOST_CHECK_THROW(env.idl("fake_test_framework/test.idl"), utils::Exception);

    BOOST_CHECK_THROW(env.idl("wrong_test/test.idl"), utils::Exception);
}

} // namespace framework_parsing_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
