#include "obj_cpp/struct_maker.h"

#include "common.h"
#include "common/common.h"
#include "common/generator.h"
#include "cpp/common.h"
#include "cpp/import_maker.h"
#include "objc/import_maker.h"
#include "objc/type_name_maker.h"
#include "objc/common.h"
#include "tpl/tpl.h"

#include "cpp/type_name_maker.h"

#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

StructMaker::StructMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    const common::EnumFieldMaker* enumFieldMaker,
    common::ImportMaker* importMaker,
    const common::DocMaker* docMaker,
    ctemplate::TemplateDictionary* rootDict,
    bool isHeader)
    : common::StructMaker(
          podDecider,
          typeNameMaker,
          enumFieldMaker,
          importMaker,
          docMaker),
      rootDict_(rootDict),
      isHeader_(isHeader),
      visited_(false),
      fieldNumber_(0)
{
}

ctemplate::TemplateDictionary* StructMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Struct& s)
{
    visited_ = true;

    if (!needGenerateNode(scope, s)) {
        if (s.kind == nodes::StructKind::Bridged && !isHeader_) {
            importMaker_->addCppRuntimeImportPath("any/ios/to_platform.h");
            importMaker_->addObjCRuntimeImportPath("NativeObject.h");
        }

        return common::StructMaker::createDictForInternalStruct(parentDict, scope, s);
    }

    FullTypeRef typeRef(scope, s.name);
    const auto& typeInfo = typeRef.info();

    importMaker_->addImportPath(filePath(typeInfo.idl, true).inAngles());
    importMaker_->addImportPath(objc::filePath(typeInfo.idl, true).inAngles());
    importMaker_->addImportPath(cpp::filePath(typeInfo, true).inAngles());

    importMaker_->addCppRuntimeImportPath("bindings/ios/to_platform.h");
    importMaker_->addCppRuntimeImportPath("bindings/ios/to_native.h");

    if (!isHeader_) {
        importMaker_->addCppRuntimeImportPath("any/ios/to_platform.h");
        importMaker_->addObjCRuntimeImportPath("NativeObject.h");
    }

    fieldNumber_ = 0;
    auto dict = common::StructMaker::make(parentDict, scope, s);

    objc::setRuntimeFrameworkPrefix(scope.idl->env, dict);

    objc::TypeNameMaker iosTNM;
    dict->SetValue("OBJC_STRUCT_NAME", iosTNM.make(typeRef));
    dict->SetValue("OBJC_INSTANCE_NAME", iosTNM.makeInstanceName(typeRef.info()));

    return dict;
}

ctemplate::TemplateDictionary* StructMaker::createDict(
    ctemplate::TemplateDictionary* /* parentDict */,
    const FullScope& scope,
    const nodes::Struct& s) const
{
    auto tplName = s.kind == nodes::StructKind::Options ?
        "options_struct.tpl" : "struct.tpl";
    auto dict = tpl::addSectionedInclude(rootDict_, "TOPLEVEL_CHILD", tplName);

    common::setNamespace(
        scope.idl->env->runtimeFramework()->cppNamespace,
        dict->AddSectionDictionary("RUNTIME_NAMESPACE"));

    dict->SetValue("RUNTIME_NAMESPACE_PREFIX",
        "::" + scope.idl->env->runtimeFramework()->cppNamespace.asPrefix("::"));

    return dict;
}

void StructMaker::fillFieldDict(
    ctemplate::TemplateDictionary* dict,
    const FullScope& scope,
    const nodes::Struct& s,
    const nodes::StructField& f)
{
    if (needGenerateNode(scope, f)) {
        dict->ShowSection("VISIBLE_FIELD");
        dict->SetValue("OBJC_METHOD_NAME_FIELD_PART", 
            fieldNumber_ == 0 ? utils::capitalizeWord(f.name) : f.name);
        ++fieldNumber_;
    } else {
        dict->SetValueAndShowSection("VALUE", fillHiddenFieldValue(scope, f), "HIDDEN_FIELD");
    }

    common::StructMaker::fillFieldDict(dict, scope, s, f);

    FullTypeRef typeRef(scope, f.typeRef);

    objc::TypeNameMaker iosTNM;
    dict->SetValue("OBJC_FIELD_TYPE", iosTNM.makeRef(typeRef));

    importMaker_->addAll(typeRef, false, false);
}

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
