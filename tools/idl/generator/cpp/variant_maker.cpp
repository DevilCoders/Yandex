#include "cpp/variant_maker.h"

#include "common/common.h"
#include "common/import_maker.h"

#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

ctemplate::TemplateDictionary* VariantMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Variant& v)
{
    if (v.customCodeLink.baseHeader) {
        importMaker_->addImportPath('<' + *v.customCodeLink.baseHeader + '>');
        return nullptr; // everything inside is considered custom-defined
    }

    importMaker_->addImportPath("<boost/variant.hpp>");
    importMaker_->addImportPath("<boost/serialization/variant.hpp>");

    return common::VariantMaker::make(parentDict, scope, v);
}

ctemplate::TemplateDictionary* VariantMaker::makeField(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Variant& v,
    const nodes::VariantField& f,
    int fieldIndex) const
{
    importMaker_->addAll(FullTypeRef(scope, f.typeRef), true);

    return common::VariantMaker::makeField(
        parentDict, scope, v, f, fieldIndex);
}

std::string VariantMaker::fieldName(
    const FullScope& scope,
    const nodes::Variant& v,
    const nodes::TypeRef& fieldTypeRef,
    bool isOptional) const
{
    auto baseName = common::VariantMaker::fieldName(
        scope, v, fieldTypeRef, isOptional);

    if (FullTypeRef(scope, fieldTypeRef).isHolding(v)) {
        importMaker_->addImportPath("<boost/variant/recursive_wrapper.hpp>");
        return "boost::recursive_wrapper<" + baseName + ">";
    } else {
        return baseName;
    }
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
