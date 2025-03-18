#include "obj_cpp/function_maker.h"

#include "common/common.h"
#include "common/function_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"
#include "cpp/common.h"
#include "cpp/import_maker.h"
#include "cpp/type_name_maker.h"
#include "obj_cpp/common.h"
#include "obj_cpp/import_maker.h"
#include "objc/common.h"
#include "objc/function_maker.h"
#include "objc/type_name_maker.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/utils.h>

#include <utility>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

ctemplate::TemplateDictionary* FunctionMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Function& f,
    bool isLambda)
{
    if (!needGenerateNode(scope, f))
        return parentDict;

    auto dict = common::FunctionMaker::make(parentDict, scope, f, isLambda);

    dict->SetValue("OBJC_FUNCTION_NAME", f.name[OBJC].first());
    if (!f.name.isDefined(OBJC) && !f.parameters.empty()) {
        dict->ShowSection("WITH");
    }

    if (isLambda) {
        dict->SetValue("LAMBDA_METHOD_SCOPE",
            scope.scope.subScope(0, scope.scope.size() - 1).asPrefix("::"));
    }

    objc::TypeNameMaker iosTNM;
    FullTypeRef resultTypeRef(scope, f.result.typeRef);
    dict->SetValue("OBJC_RESULT_TYPE", iosTNM.makeRef(resultTypeRef));

    if (f.result.typeRef.id == nodes::TypeId::Custom) {
        const auto& typeInfo = resultTypeRef.info();

        auto i = resultTypeRef.as<nodes::Interface>();
        if (i && i->ownership != nodes::Interface::Ownership::Weak) {
            dict->ShowSection("INIT_ALLOC_RESULT");
            dict->SetValue("OBJC_RESULT_CLASS", iosTNM.make(resultTypeRef));
        } else if (resultTypeRef.is<nodes::Enum>() &&
                !f.result.typeRef.isOptional) {
            dict->ShowSection("STATIC_CAST_RESULT");
        } else {
            auto l = resultTypeRef.as<nodes::Listener>();
            if (l) {
                auto resultDict =
                    dict->AddSectionDictionary("LISTENER_RESULT");
                resultDict->SetValue("LISTENER_BINDING_TYPE_NAME",
                    cpp::listenerBindingTypeName("ios", typeInfo));

                visitedListenerReturnValue_ = true;
                if (l->isStrongRef) {
                    visitedStrongRefListenerReturnValue_ = true;
                }
            } else {
                dict->ShowSection("SIMPLE_RESULT");
            }
        }
    } else if (f.result.typeRef.id != nodes::TypeId::Void) {
        dict->ShowSection("SIMPLE_RESULT");
    }

    return dict;
}

ctemplate::TemplateDictionary* FunctionMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /* scope */,
    bool /* isLambda */) const
{
    return parentDict->AddSectionDictionary("METHOD");
}

void FunctionMaker::makeParameter(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Function& function)
{
    const auto& p = function.parameters[parameterIndexInIdl_];

    auto cppParam = parentDict->AddSectionDictionary("CPP_PARAM");

    std::size_t parameterIndexInIdl = parameterIndexInIdl_;
    std::size_t parameterIndexInName = parameterIndexInName_;
    cpp::FunctionMaker::makeParameter(cppParam, scope, function);
    parameterIndexInIdl_ = parameterIndexInIdl;
    parameterIndexInName_ = parameterIndexInName;

    std::string objcParamName = p.name;
    bool isEnum = false, isStrongInterface = false, isSharedInterface = false;
    bool isParamListener = false;

    FullTypeRef typeRef(scope, p.typeRef);
    if (p.typeRef.id == nodes::TypeId::Custom) {
        const auto& typeInfo = typeRef.info();

        auto l = typeRef.as<nodes::Listener>();
        if (l) {
            if (l->isLambda) {
                importMaker_->addAll(typeRef, false, isHeader_);

                auto dict = parentDict->AddSectionDictionary("PARAM");

                auto listenerScopeSuffix = typeInfo.scope.original();
                if (!listenerScopeSuffix.isEmpty()) {
                    --listenerScopeSuffix;
                }
                dict->SetValue("LISTENER_SCOPE",
                    (typeInfo.idl->cppNamespace + listenerScopeSuffix).asPrefix("::"));

                dict->SetValue("PARAM_TYPE", typeNameMaker_->makeRef(typeRef));

                objc::TypeNameMaker iosTNM;
                dict->SetValue("OBJC_PARAM_TYPE", iosTNM.makeRef(typeRef));

                auto name = iosTNM.makeInstanceName(typeInfo);
                objc::setObjCMethodNameParamPart(
                    dict, name, function, parameterIndexInName_);

                dict->SetValue("PARAM_NAME", name);

                if (p.defaultValue) {
                    dict->SetValueAndShowSection(
                        "VALUE", *p.defaultValue, "DEFAULT_VALUE");
                }

                dict->ShowSection(
                    podDecider_->isPod(typeRef) ? "POD" : "NOT_POD");
                importMaker_->addAll(typeRef, false);

                for (const auto& m : l->functions) {
                    auto paramDict = dict->AddSectionDictionary("PARAM_LAMBDA");

                    auto methodScope = *p.typeRef.name;
                    --methodScope;
                    paramDict->SetValue("PARAM_TYPE",
                        methodScope.asPrefix("::") +
                            utils::capitalizeWord(m.name[targetLang_].first()));
                    paramDict->SetValue("OBJC_PARAM_TYPE", iosTNM.makeRef(typeRef));
                    paramDict->ShowSection(
                        podDecider_->isPod(typeRef) ? "POD" : "NOT_POD");
                    if (p.defaultValue) {
                        paramDict->SetValueAndShowSection(
                            "VALUE", *p.defaultValue, "DEFAULT_VALUE");
                    }
                    paramDict->SetValue("PARAM_NAME", m.name[targetLang_].first());
                    paramDict->SetValue("LAMBDA_PARAM_NAME", name);

                    paramDict->ShowSection("CONST_PARAM");
                    paramDict->ShowSection("REF_PARAM");
                }
                objcParamName = l->name[OBJC];
            } else {
                importMaker_->addCppRuntimeImportPath("ios/object.h");

                auto dict =
                    createParameterDict(parentDict, scope, p, function);
                dict->ShowSection("PARAM_LISTENER");

                // PARAM_TYPE value for a listener is wrapped in shared_ptr,
                // so we create variables with clean listener scope and name:
                dict->SetValue("LISTENER_TYPE_NAME",
                    cpp::fullName(typeInfo.fullNameAsScope[CPP]));

                dict->SetValue("LISTENER_NAME", p.typeRef.name->last());

                dict->SetValue("LISTENER_BINDING_TYPE_NAME",
                    cpp::listenerBindingTypeName("ios", typeInfo));

                if (l->isStrongRef) {
                    visitedStrongRefListenerParameter_ = true;

                    dict->AddSectionDictionary("STRONG_LISTENER_PARAM");
                } else {
                    dict->AddSectionDictionary("WEAK_LISTENER_PARAM");
                }
            }

            isParamListener = true;
        } else {
            auto i = typeRef.as<nodes::Interface>();
            if (i) {
                isStrongInterface =
                    i->ownership == nodes::Interface::Ownership::Strong;
                isSharedInterface =
                    i->ownership == nodes::Interface::Ownership::Shared;
            } else {
                isEnum = typeRef.is<nodes::Enum>();
            }
        }
    }

    common::addNullAsserts(parentDict, typeRef, objcParamName);

    if (isParamListener) {
        return;
    }
    auto paramDict = createParameterDict(parentDict, scope, p, function);
    if (typeRef.isError()) {
        paramDict->ShowSection("PARAM_NSERROR");
    } else if (isEnum && !p.typeRef.isOptional) {
        paramDict->ShowSection("PARAM_ENUM");
        paramDict->ShowSection("TO_NATIVE");
    } else {
        if (isStrongInterface || isSharedInterface) {
            paramDict->ShowSection("ALLOC_INIT_INTERFACE");
            if (isStrongInterface) {
                paramDict->ShowSection("MOVE");
            }
        } else {
            paramDict->ShowSection("TO_PLATFORM");
        }
        paramDict->ShowSection("TO_NATIVE");
    }
}

ctemplate::TemplateDictionary* FunctionMaker::createParameterDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::FunctionParameter& p,
    const nodes::Function& function)
{
    auto dict = cpp::FunctionMaker::createParameterDict(
        parentDict, scope, p, function);

    FullTypeRef typeRef(scope, p.typeRef);

    objc::TypeNameMaker iosTNM;
    dict->SetValue("OBJC_PARAM_TYPE", iosTNM.makeRef(typeRef));

    if (p.typeRef.id == nodes::TypeId::Custom) {
        dict->SetValue("OBJC_INTERFACE_TYPE", iosTNM.make(typeRef));
    }

    objc::setObjCMethodNameParamPart(
        dict, p.name, function, parameterIndexInName_);

    return dict;
}

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
