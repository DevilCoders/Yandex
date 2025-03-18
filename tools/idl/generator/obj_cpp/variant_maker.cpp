#include "obj_cpp/variant_maker.h"

#include "common/common.h"
#include "common/import_maker.h"
#include "common/generator.h"
#include "cpp/common.h"
#include "cpp/import_maker.h"
#include "cpp/type_name_maker.h"
#include "obj_cpp/common.h"
#include "objc/type_name_maker.h"
#include "objc/common.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

VariantMaker::VariantMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    const common::DocMaker* docMaker,
    ctemplate::TemplateDictionary* rootDict)
    : common::VariantMaker(podDecider, typeNameMaker, importMaker, docMaker),
      rootDict_(rootDict),
      visited_(false)
{
}

ctemplate::TemplateDictionary* VariantMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Variant& v)
{
    visited_ = true;

    if (!needGenerateNode(scope, v))
        return parentDict;

    auto *dict = common::VariantMaker::make(parentDict, scope, v);

    FullTypeRef typeRef(scope, v.name);

    const auto& typeInfo = typeRef.info();
    importMaker_->addImportPath(filePath(typeInfo.idl, true).inAngles());
    importMaker_->addImportPath(objc::filePath(typeInfo.idl, true).inAngles());
    importMaker_->addImportPath(cpp::filePath(typeInfo, true).inAngles());

    importMaker_->addCppRuntimeImportPath("bindings/ios/to_platform_fwd.h");
    importMaker_->addCppRuntimeImportPath("bindings/ios/to_native_fwd.h");
    importMaker_->addImportPath("<type_traits>");

    dict->SetValue("OBJC_VARIANT_NAME", objc::TypeNameMaker().make(typeRef));

    common::setNamespace(scope.idl->cppNamespace, dict);

    return dict;
}

ctemplate::TemplateDictionary* VariantMaker::createDict(
    ctemplate::TemplateDictionary* /*parentDict*/,
    const FullScope& scope) const
{
    auto dict = tpl::addSectionedInclude(rootDict_, "TOPLEVEL_CHILD", "variant.tpl");

    common::setNamespace(
        scope.idl->env->runtimeFramework()->cppNamespace,
        dict->AddSectionDictionary("RUNTIME_NAMESPACE"));

    dict->SetValue("RUNTIME_NAMESPACE_PREFIX",
        "::" + scope.idl->env->runtimeFramework()->cppNamespace.asPrefix("::"));

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

    dict->SetValue("BIND_CLASS_TYPE",
        cpp::TypeNameMaker().makeRef(FullTypeRef(scope, f.typeRef)));

    return dict;
}

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
