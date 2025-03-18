#include "java/annotation_addition.h"

#include "common/import_maker.h"

#include <yandex/maps/idl/full_type_ref.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

void addNullabilityAnnotation(
    ctemplate::TemplateDictionary* dict,
    common::ImportMaker* importMaker,
    const FullTypeRef& typeRef,
    bool addImport,
    const std::string& sectionPrefix)
{
    if (typeRef.isOptional()) {
        if (addImport) {
            addNullableImport(importMaker);
        }

        dict->ShowSection(sectionPrefix + "NULLABLE");
    } else if (typeRef.isJavaNullable()) {
        if (addImport) {
            addNonNullImport(importMaker);
        }

        dict->ShowSection(sectionPrefix + "NONNULL");
    }
}

void addNullableImport(common::ImportMaker* importMaker)
{
    importMaker->addImportPath("androidx.annotation.Nullable");
}

void addNonNullImport(common::ImportMaker* importMaker)
{
    importMaker->addImportPath("androidx.annotation.NonNull");
}

void addThreadRestrictionAnnotationImport(
    common::ImportMaker* importMaker,
    nodes::Function::ThreadRestriction restriction)
{
    switch (restriction) {
        case nodes::Function::ThreadRestriction::Ui:
            importMaker->addImportPath("androidx.annotation.UiThread");
            break;
        case nodes::Function::ThreadRestriction::Bg:
            importMaker->addImportPath("androidx.annotation.WorkerThread");
            break;
        case nodes::Function::ThreadRestriction::None:
            importMaker->addImportPath("androidx.annotation.AnyThread");
            break;
    }
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
