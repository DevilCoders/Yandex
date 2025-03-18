#include "objc/variant_maker.h"

#include "objc/common.h"
#include "objc/serialization_addition.h"
#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

VariantMaker::VariantMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    const common::DocMaker* docMaker,
    ctemplate::TemplateDictionary* rootDict)
    : common::VariantMaker(
          podDecider,
          typeNameMaker,
          importMaker,
          docMaker),
      rootDict_(rootDict)
{
}

ctemplate::TemplateDictionary* VariantMaker::make(
    ctemplate::TemplateDictionary* /* parentDict */,
    FullScope scope,
    const nodes::Variant& v)
{
    if (!needGenerateNode(scope, v))
        return nullptr;

    if (scope.scope.size() > 0) {
        importMaker_->addForwardDeclaration(
            FullTypeRef(scope, v.name), filePath(scope.idl, true));
    }

    auto dict = common::VariantMaker::make(rootDict_, scope, v);
    setRuntimeFrameworkPrefix(scope.idl->env, dict);
    return dict;
}

ctemplate::TemplateDictionary* VariantMaker::makeField(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Variant& v,
    const nodes::VariantField& f,
    int fieldIndex) const
{
    auto dict = common::VariantMaker::makeField(
        parentDict, scope, v, f, fieldIndex);
    addFieldSerialization(podDecider_, typeNameMaker_, importMaker_, dict,
        FullTypeRef(scope, f.typeRef));

    FullTypeRef fieldTypeRef(scope, f.typeRef);
    if (supportsOptionalAnnotation(fieldTypeRef)) {
        dict->ShowSection("NON_OPTIONAL");
    }
    return dict;
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
