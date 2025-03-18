#pragma once

#include <yandex/maps/idl/framework.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {

/**
 * @param filePath needed for error messages only
 */
Framework parseFramework(
    const std::string& filePath,
    const std::string& fileContents);

} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
