#pragma once

#include <yandex/maps/idl/env.h>
#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/utils/paths.h>

#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {

Environment simpleEnv(
    utils::SearchPaths&& frameworkPaths,
    utils::SearchPaths&& idlPaths,
    bool disableInternalChecks);

/**
 * Generates trivial documentation based on given documentation block.
 */
std::string generateIdlDoc(const nodes::DocBlock& docBlock);

/**
 * Checks if given strings are equal and, if not, produces boost test message
 * with the number of line where they were different.
 */
void checkStringsEqualByLine(
    const std::string& actual,
    const std::string& expected);

} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
