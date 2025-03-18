#pragma once

#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/scope.h>

#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

/**
 * Holds entire .idl file contents.
 */
struct Root {
    std::string objcInfix;

    /**
     * Imported .idl file paths.
     */
    std::vector<std::string> imports;

    /**
     * On file's top level there can be interfaces, structures, enums.
     */
    Nodes nodes;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
