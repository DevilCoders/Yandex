#pragma once

#include "common/type_name_maker.h"

#include <yandex/maps/idl/env.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/type_info.h>
#include <yandex/maps/idl/utils/paths.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

/**
 * Constructs .h or .m path corresponding to given .idl file (file whose name
 * is constructed from .idl's name).
 */
utils::Path filePath(const Idl* idl, bool isHeader);

/**
 * Constructs .h or .m path corresponding to top-level type with given name
 * space and name (type must not be enum or lambda listener because they go
 * into .idl-corresponding files).
 */
utils::Path filePath(const Idl* idl, const nodes::Name& name, bool isHeader);

/**
 * Constructs .h or .m path where given type will be placed.
 */
utils::Path filePath(const TypeInfo& typeInfo, bool isHeader);

/**
 * Aligns function arguments around ':' delimiter in Objective-C style.
 */
std::string alignParameters(const std::string& text);

std::string runtimeFrameworkPrefix(const Environment* env);
void setRuntimeFrameworkPrefix(
    const Environment* env,
    ctemplate::TemplateDictionary* dict);

/**
 * Check if is possible to add flag for determine nullability of objects
 * like variables or functions
 * @return - true if it is possible to put the flag (poiner type)
 */
bool supportsOptionalAnnotation(
    const FullTypeRef &typeRef);

/**
 * Check should we write NULLABLE against NONNULL annotaion
 */
bool hasNullableAnnotation(
            const FullTypeRef &typeRef);

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
