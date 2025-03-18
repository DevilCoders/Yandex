#pragma once

#include "common/import_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

#include <set>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

/**
 * Adds struct serialization for Java.
 */
void addStructSerialization(
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Struct& s);

/**
 * Adds struct or variant field serialization for Java.
 */
void addFieldSerialization(
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* paramsDict,
    const FullTypeRef& fieldTypeRef);

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
