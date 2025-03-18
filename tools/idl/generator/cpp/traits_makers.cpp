#include "cpp/traits_makers.h"

#include "common/common.h"
#include "common/interface_maker.h"
#include "common/type_name_maker.h"
#include "cpp/import_maker.h"
#include "cpp/type_name_maker.h"
#include "jni_cpp/jni.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

namespace {

void makeTraitTypeCondition(
    const TypeInfo& /* typeInfo */,
    ctemplate::TemplateDictionary* dict,
    const nodes::Enum& e)
{
    if (e.isBitField) {
        dict->ShowSection("IS_BIT_FIELD");
    }
}
void makeTraitTypeCondition(
    const TypeInfo& typeInfo,
    ctemplate::TemplateDictionary* dict,
    const nodes::Interface& i)
{
    auto interfaceDict = dict->AddSectionDictionary("IS_INTERFACE");

    const auto& topMostBase = *common::topMostBaseTypeInfo(&typeInfo);
    interfaceDict->SetValue(
        "TOP_MOST_BASE_TYPE", fullName(topMostBase.fullNameAsScope[CPP]));

    if (i.ownership == nodes::Interface::Ownership::Strong) {
        interfaceDict->ShowSection("IS_STRONG");
    } else if (i.ownership == nodes::Interface::Ownership::Shared) {
        interfaceDict->ShowSection("IS_SHARED");
    } else {
        interfaceDict->ShowSection("IS_WEAK");
    }

    interfaceDict->SetValue("JAVA_BINDING_UNDECORATED_NAME",
        jni_cpp::jniTypeRef(typeInfo, true, true));
}
void makeTraitTypeCondition(
    const TypeInfo& /* typeInfo */,
    ctemplate::TemplateDictionary* dict,
    const nodes::Listener& l)
{
    dict->ShowSection("IS_LISTENER");
    dict->ShowSection(l.isStrongRef ? "IS_STRONG" : "IS_WEAK");
}
void makeTraitTypeCondition(
    const TypeInfo& /* typeInfo */,
    ctemplate::TemplateDictionary* dict,
    const nodes::Struct& s)
{
    dict->ShowSection("IS_STRUCT");
    if (s.kind == nodes::StructKind::Bridged) {
        dict->ShowSection("IS_BRIDGED");
    }
}
void makeTraitTypeCondition(
    const TypeInfo& /* typeInfo */,
    ctemplate::TemplateDictionary* dict,
    const nodes::Variant& /* v */)
{
    dict->ShowSection("IS_VARIANT");
}

} // namespace

BindingTraitsMaker::BindingTraitsMaker(
    ctemplate::TemplateDictionary* rootDict,
    const Idl* idl,
    common::ImportMaker* importMaker)
    : rootDict_(rootDict),
      idl_(idl),
      importMaker_(importMaker)
{
    importMaker_->addCppRuntimeImportPath("bindings/traits.h");
}

template <typename IdlNode>
void BindingTraitsMaker::makeTraitsFor(const IdlNode& n)
{
    if (!dict_) {
        dict_ = rootDict_->AddSectionDictionary("TRAITS");
        common::setNamespace(
            idl_->env->runtimeFramework()->cppNamespace,
            dict_->AddSectionDictionary("RUNTIME_NAMESPACE"));
    }

    auto typeDict = dict_->AddSectionDictionary("TYPE");

    const auto& typeInfo = idl_->type(scope_, Scope(n.name.original()));

    typeDict->SetValue("TYPE_NAME", fullName(typeInfo.fullNameAsScope[CPP]));

    makeTraitTypeCondition(typeInfo, typeDict, n);

    typeDict->SetValue("JAVA_UNDECORATED_NAME",
        jni_cpp::jniTypeRef(typeInfo, true, false));
    typeDict->SetValue("OBJECTIVE_C_NAME",
        typeInfo.fullNameAsScope[OBJC].asString(""));
    typeDict->SetValue("NATIVE_NAME", cpp::nativeName(typeInfo));
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
