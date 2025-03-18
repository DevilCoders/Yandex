#pragma once

#include <yandex/maps/idl/generator/output_file.h>

#include <yandex/maps/idl/idl.h>

#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

/**
 * Generates Objective-C .h and .mm files.
 */
std::vector<OutputFile> generate(const Idl* idl);

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
