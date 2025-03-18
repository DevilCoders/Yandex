#include "obj_cpp/listener_maker.h"

#include "common/common.h"
#include "common/generator.h"
#include "common/import_maker.h"
#include "cpp/common.h"
#include "cpp/type_name_maker.h"
#include "obj_cpp/common.h"
#include "objc/common.h"
#include "objc/type_name_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

ListenerMaker::ListenerMaker(
    const common::TypeNameMaker* typeNameMaker,
    const common::DocMaker* docMaker,
    common::FunctionMaker* functionMaker,
    ctemplate::TemplateDictionary* rootDict,
    bool isHeader,
    common::ImportMaker* importMaker)
    : common::ListenerMaker(typeNameMaker, docMaker, functionMaker, true),
      rootDict_(rootDict),
      isHeader_(isHeader),
      importMaker_(importMaker),
      visited_(false)
{
}

ctemplate::TemplateDictionary* ListenerMaker::make(
    ctemplate::TemplateDictionary* /* parentDict */,
    FullScope scope,
    const nodes::Listener& l)
{
    FullTypeRef typeRef(scope, l.name);
    const auto& typeInfo = typeRef.info();
    if (isHeader_) {
        if (scope.scope.isEmpty()) {
            importMaker_->addImportPath(
                cpp::filePath(typeInfo, true).inAngles());
            importMaker_->addImportPath(
                objc::filePath(typeInfo.idl, true).inAngles());
        }
    } else {
        importMaker_->addImportPath(
            obj_cpp::filePath(typeInfo.idl, true).inAngles());
        importMaker_->addCppRuntimeImportPath("verify_and_run.h");
    }

    visited_ = true;

    if (!needGenerateNode(scope, l))
        return nullptr;

    // ToNative / ToPlatform
    if (typeRef.isClassicListener()) {
        importMaker_->addCppRuntimeImportPath("bindings/ios/to_native.h");
        importMaker_->addCppRuntimeImportPath("bindings/ios/to_platform.h");

        auto bindingDict = rootDict_->AddSectionDictionary("LISTENER_TO_NATIVE_TO_PLATFORM");

        bindingDict->SetValue("INSTANCE_NAME", typeNameMaker_->makeInstanceName(typeRef.info()));
        bindingDict->SetValue("CPP_TYPE_NAME", typeNameMaker_->make(typeRef));
        bindingDict->SetValue("OBJC_TYPE_NAME", objc::TypeNameMaker().makeRef(typeRef));
        bindingDict->SetValue("BINDING_TYPE_NAME", cpp::listenerBindingTypeName("ios", typeInfo));

        common::setNamespace(
            scope.idl->env->runtimeFramework()->cppNamespace,
            bindingDict->AddSectionDictionary("RUNTIME_NAMESPACE"));
    }

    // Binding implementation
    auto dict = common::ListenerMaker::make(rootDict_, scope, l);
    dict->SetValue("OBJC_TYPE_NAME", objc::TypeNameMaker().make(typeRef));
    return dict;
}

ctemplate::TemplateDictionary* ListenerMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Listener& l) const
{
    auto dict = common::ListenerMaker::createDict(parentDict, scope, l);

    dict->SetValue("RUNTIME_NAMESPACE_PREFIX",
        "::" + scope.idl->env->runtimeFramework()->cppNamespace.asPrefix("::"));

    return dict;
}

ctemplate::TemplateDictionary* ListenerMaker::makeFunction(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Listener& l,
    const nodes::Function& f)
{
    if (l.isLambda) {
        auto methodDict =
            common::ListenerMaker::makeFunction(parentDict, scope, l, f);

        bool isCurrentMethodBefore = true;
        int paramBeforeNum = 0, paramMethodNum = 0, paramAfterNum = 0;
        for (const auto& function : l.functions) {
            if (&function != &f && isCurrentMethodBefore) {
                for (size_t i = 0; i < function.parameters.size(); ++i) {
                    methodDict->AddSectionDictionary("NIL_BEFORE");
                    ++paramBeforeNum;
                }
            } else if (&function == &f) {
                isCurrentMethodBefore = false;
                paramMethodNum = function.parameters.size();
            } else if (&function != &f && !isCurrentMethodBefore) {
                for (size_t i = 0; i < function.parameters.size(); ++i) {
                    methodDict->AddSectionDictionary("NIL_AFTER");
                    ++paramAfterNum;
                }
            }
        }

        if (paramBeforeNum != 0 &&
                paramAfterNum != 0 &&
                paramMethodNum == 0) {
            methodDict->ShowSection("COMMA_AFTER");
        } else if (paramBeforeNum + paramMethodNum != 0 &&
                paramAfterNum != 0) {
            methodDict->ShowSection("COMMA_AFTER");
        } else if (paramBeforeNum != 0 &&
                paramMethodNum + paramAfterNum != 0) {
            methodDict->ShowSection("COMMA_BEFORE");
        }
        return methodDict;
    } else {
        return common::ListenerMaker::makeFunction(parentDict, scope, l, f);
    }

    return nullptr;
}

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
