#pragma once

#include <yandex/maps/idl/generator/output_file.h>

#include <yandex/maps/idl/idl.h>

#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

/**
 * Generates Objective-C .h and .m files.
 */
std::vector<OutputFile> generate(const Idl* idl);

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
