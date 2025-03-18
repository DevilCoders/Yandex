#pragma once

#include "common/import_maker.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/function.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

/**
 * Generates @Nullable and @NonNull annotations (by adding NULLABLE or NONNULL sections) if needed,
 * and adds necessary imports.
 */
void addNullabilityAnnotation(
    ctemplate::TemplateDictionary* dict,
    common::ImportMaker* importMaker,
    const FullTypeRef& typeRef,
    bool addImport = true,
    const std::string& sectionPrefix = "");

void addNullableImport(common::ImportMaker* importMaker);
void addNonNullImport(common::ImportMaker* importMaker);

void addThreadRestrictionAnnotationImport(
    common::ImportMaker* importMaker,
    nodes::Function::ThreadRestriction restriction);

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
