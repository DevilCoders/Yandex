#include "jni_cpp/struct_maker.h"
#include "jni_cpp/jni.h"

#include "common/common.h"
#include "cpp/import_maker.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/utils.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/targets.h>

namespace yandex::maps::idl::generator::jni_cpp {

StructMaker::StructMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    const common::EnumFieldMaker* enumFieldMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* rootDict,
    bool isHeader)
    : cpp::StructMaker(
          podDecider,
          typeNameMaker,
          enumFieldMaker,
          importMaker,
          nullptr,
          false,
          true),
      rootDict_(rootDict),
      isHeader_(isHeader)
{
}

ctemplate::TemplateDictionary* StructMaker::make(
    ctemplate::TemplateDictionary* /*parentDict*/,
    FullScope scope,
    const nodes::Struct& s)
{
    if (!needGenerateNode(scope, s)) {
        if (s.kind == nodes::StructKind::Bridged && isHeader_) {
            importMaker_->addCppRuntimeImportPath("any/android/to_platform.h");
        }

        return common::StructMaker::createDictForInternalStruct(rootDict_, scope, s);
    }

    countFields_ = s.nodes.count<nodes::StructField>();
    countNonInternalFields_ = s.nodes.count<nodes::StructField>([&] (const nodes::StructField& field) {
        return !hasInternalDoc(field);
    });
    numberFillingField_ = 0;

    auto dict = cpp::StructMaker::make(rootDict_, scope, s);

    const auto& typeInfo = scope.type(s.name.original());
    dict->SetValue("STRUCT_NAME_IN_FUNCTION_NAME",
        mangleJniTypeRefs(jniTypeRef(typeInfo, true, false)));

    if (s.kind != nodes::StructKind::Bridged) {
        return dict;
    }

    importMaker_->addCppRuntimeImportPath("any/android/to_platform.h");
    importMaker_->addCppRuntimeImportPath(
        "bindings/android/internal/new_serialization.h");

    return dict;
}

ctemplate::TemplateDictionary* StructMaker::makeField(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Struct& s,
    const nodes::StructField& f)
{
    return cpp::StructMaker::makeField(parentDict, scope, s, f);
}

ctemplate::TemplateDictionary* StructMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Struct& /* s */) const
{
    auto dict = parentDict->AddSectionDictionary("STRUCT");

    common::setNamespace(
        scope.idl->env->runtimeFramework()->cppNamespace,
        dict->AddSectionDictionary("RUNTIME_NAMESPACE"));

    return dict;
}

bool StructMaker::lastFillingVisibleField(
    const FullScope& scope,
    const nodes::StructField& f)
{
    if (!scope.idl->env->config.isPublic) {
        return numberFillingField_ == countFields_ - 1;
    }

    REQUIRE(!hasInternalDoc(f), "Internal field can not be visible field at public release");

    return numberFillingField_ == countNonInternalFields_ - 1;
}

void StructMaker::fillFieldDict(
    ctemplate::TemplateDictionary* dict,
    const FullScope& scope,
    const nodes::Struct& s,
    const nodes::StructField& f)
{
    if (needGenerateNode(scope, f)) {
        dict->ShowSection("VISIBLE_FIELD");
        if (!lastFillingVisibleField(scope, f)) {
            dict->ShowSection("FIELD_SEP");
            numberFillingField_++;
        }
    } else {
        dict->SetValueAndShowSection("VALUE", fillHiddenFieldValue(scope, f), "HIDDEN_FIELD");
    }

    cpp::StructMaker::fillFieldDict(dict, scope, s, f);

    FullTypeRef typeRef(scope, f.typeRef);
    dict->SetValue("JNI_FIELD_TYPE", jniTypeName(typeRef, false));
    dict->SetValue("JNI_CLASS_TYPE", jniTypeRef(typeRef, false));
}

} // namespace yandex::maps::idl::generator::jni_cpp
