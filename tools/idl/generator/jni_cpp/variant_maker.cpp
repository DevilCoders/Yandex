#include "variant_maker.h"

#include "common/common.h"
#include "cpp/type_name_maker.h"
#include "jni_cpp/jni.h"

#include <yandex/maps/idl/utils.h>
#include <yandex/maps/idl/framework.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

VariantMaker::VariantMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* rootDict)
    : common::VariantMaker(podDecider, typeNameMaker, importMaker, nullptr),
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

    auto* dict = common::VariantMaker::make(rootDict_, scope, v);

    const auto& typeInfo = scope.type(v.name.original());
    dict->SetValue("JNI_VARIANT_NAME", jniTypeRef(typeInfo, true, false));

    return dict;
}

ctemplate::TemplateDictionary* VariantMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope) const
{
    auto dict = parentDict->AddSectionDictionary("VARIANT");
    common::setNamespace(
        scope.idl->env->runtimeFramework()->cppNamespace,
        dict->AddSectionDictionary("RUNTIME_NAMESPACE"));
    return dict;
}

ctemplate::TemplateDictionary* VariantMaker::makeField(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Variant& v,
    const nodes::VariantField& f,
    int fieldIndex) const
{
    auto* dict = common::VariantMaker::makeField(
        parentDict, scope, v, f, fieldIndex);

    dict->SetValue("JNI_CLASS_TYPE",
        jniTypeRef(FullTypeRef(scope, f.typeRef), false));

    return dict;
}

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
