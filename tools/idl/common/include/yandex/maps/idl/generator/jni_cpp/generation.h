#pragma once

#include <yandex/maps/idl/generator/output_file.h>

#include <yandex/maps/idl/idl.h>

#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

/**
 * Generates JNI .cpp files for given .idl file.
 */
std::vector<OutputFile> generate(const Idl* idl);

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
