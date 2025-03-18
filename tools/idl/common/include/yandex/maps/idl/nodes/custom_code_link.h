#pragma once

#include <optional>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

/**
 * Represents connections with custom-written code. Useful when Idl data types
 * are declared in custom headers, have custom protobuf converters...
 */
struct CustomCodeLink {
    /**
     * C++ type definition header.
     */
    std::optional<std::string> baseHeader;

    /**
     * Protobuf converter header.
     */
    std::optional<std::string> protoconvHeader;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
