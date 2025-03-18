#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/type_info.h>
#include <yandex/maps/idl/utils/paths.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

/**
 * Constructs .h or .mm path corresponding to given .idl file (file whose name
 * is constructed from .idl's name).
 */
utils::Path filePath(const Idl* idl, bool isHeader);

/**
 * Constructs .h or .mm path corresponding to top-level type with given name
 * space and name (type must not be enum or lambda listener because they go
 * into .idl-corresponding files).
 */
utils::Path filePath(const Idl* idl, const nodes::Name& name, bool isHeader);

/**
 * Constructs .h or .mm path where given type will be placed.
 */
utils::Path filePath(const TypeInfo& typeInfo, bool isHeader);

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
