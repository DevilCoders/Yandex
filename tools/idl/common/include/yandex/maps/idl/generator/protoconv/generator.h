#pragma once

#include <yandex/maps/idl/generator/output_file.h>

#include <yandex/maps/idl/idl.h>

#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

const char* const HEADER_SUFFIX = ".conv.h";
const char* const CPP_SUFFIX = ".conv.cpp";

/**
 * Generates Protobuf converters for given .idl file.
 */
std::vector<OutputFile> generate(const Idl* idl);

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
