#pragma once

#include <yandex/maps/idl/customizable.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

/**
 * Holds Idl type's name, with possible target-specific overrides.
 *
 * Note, there is no way to specify C++-specific names, they always coincide
 * with "original" Idl names.
 */
using Name = CustomizableValue<std::string>;

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
