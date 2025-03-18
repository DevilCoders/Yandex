#pragma once

#include <array>

namespace yandex {
namespace maps {
namespace idl {

const char* const CPP = "cpp";
const char* const CS = "cs";
const char* const JAVA = "java";
const char* const OBJC = "objc";

const std::array<const char* const, 4> ALL_TARGETS =
    { { CPP, CS, JAVA, OBJC } };

} // namespace idl
} // namespace maps
} // namespace yandex
