#include "java/variant_maker.h"

#include "common/import_maker.h"
#include "common/type_name_maker.h"
#include "java/annotation_addition.h"
#include "java/serialization_addition.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

ctemplate::TemplateDictionary* VariantMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Variant& v)
{
    addNullableImport(importMaker_);
    addNonNullImport(importMaker_);
    return common::VariantMaker::make(parentDict, scope, v);
}

ctemplate::TemplateDictionary* VariantMaker::makeField(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Variant& v,
    const nodes::VariantField& f,
    int fieldIndex) const
{
    auto dict = common::VariantMaker::makeField(parentDict, scope, v, f, fieldIndex);
    FullTypeRef typeRef(scope, f.typeRef);
    addNullabilityAnnotation(dict, importMaker_, typeRef);
    addFieldSerialization(typeNameMaker_, importMaker_, dict, typeRef);
    return dict;
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
