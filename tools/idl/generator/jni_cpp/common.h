#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/type_info.h>
#include <yandex/maps/idl/utils/paths.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

utils::Path filePath(const Idl* idl, bool isHeader);
utils::Path filePath(const TypeInfo& typeInfo, bool isHeader);

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
