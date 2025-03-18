#include "test_helpers.h"

namespace yandex {
namespace maps {
namespace idl {

BOOST_AUTO_TEST_CASE(validator_error_checking_test)
{
    auto root = makeRoot(
        makeEnum("Duplicates", { "F", "G", "F" })
    );

    // TODO: add tests.
    // Especially, move here all common-level tests from parser and generator.
}

} // namespace idl
} // namespace maps
} // namespace yandex
