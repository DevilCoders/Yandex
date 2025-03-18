#include "objc/function_maker.h"

#include "common/common.h"
#include "common/import_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"
#include "objc/common.h"

#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

ctemplate::TemplateDictionary* FunctionMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Function& f,
    bool isLambda)
{
    if (!needGenerateNode(scope, f))
        return parentDict;

    auto dict = common::FunctionMaker::make(parentDict, scope, f, isLambda);

    if (!f.name.isDefined(OBJC) && !f.parameters.empty()) {
        dict->ShowSection("WITH");
    }

    FullTypeRef resultTypeRef(scope, f.result.typeRef);
    if (supportsOptionalAnnotation(resultTypeRef)) {
        dict->ShowSection(hasNullableAnnotation(resultTypeRef) ? "OPTIONAL_RESULT" : "NON_OPTIONAL_RESULT");
    }

    return dict;
}

void FunctionMaker::makeParameter(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Function& function)
{
    const auto& p = function.parameters[parameterIndexInIdl_];
    if (p.typeRef.id == nodes::TypeId::Custom) {
        FullTypeRef typeRef(scope, p.typeRef);

        if (typeRef.isLambdaListener()) {
            importMaker_->addAll(typeRef, false, true);

            auto dict = parentDict->AddSectionDictionary("PARAM");

            dict->SetValue("PARAM_TYPE", typeNameMaker_->makeRef(typeRef));
            if (supportsOptionalAnnotation(typeRef)) {
                dict->ShowSection(hasNullableAnnotation(typeRef) ? "OPTIONAL" : "NON_OPTIONAL");
            }

            auto name = typeNameMaker_->makeInstanceName(typeRef.info());
            dict->SetValue("PARAM_NAME", name);

            setObjCMethodNameParamPart(
                dict, name, function, parameterIndexInName_);

            if (p.defaultValue) {
                dict->SetValueAndShowSection(
                    "VALUE", *p.defaultValue, "DEFAULT_VALUE");
            }

            dict->ShowSection(podDecider_->isPod(typeRef) ? "POD" : "NOT_POD");
            return;
        }
    }

    createParameterDict(parentDict, scope, p, function);
}

ctemplate::TemplateDictionary* FunctionMaker::createParameterDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::FunctionParameter& p,
    const nodes::Function& function)
{
    auto dict = common::FunctionMaker::createParameterDict(
        parentDict, scope, p, function);

    FullTypeRef typeRef(scope, p.typeRef);

    if (supportsOptionalAnnotation(typeRef)) {
        dict->ShowSection(hasNullableAnnotation(typeRef) ? "OPTIONAL" : "NON_OPTIONAL");
    }

    setObjCMethodNameParamPart(
        dict, p.name, function, parameterIndexInName_);

    return dict;
}

void setObjCMethodNameParamPart(
    ctemplate::TemplateDictionary* dict,
    const std::string& value,
    const nodes::Function& function,
    std::size_t& parameterIndexInName)
{
    if (function.name.isDefined(OBJC)) {
        if (parameterIndexInName > 0) {
            dict->SetValue("OBJC_METHOD_NAME_PARAM_PART",
               function.name[OBJC][parameterIndexInName]);
        }
    } else {
        dict->SetValue("OBJC_METHOD_NAME_PARAM_PART",
            parameterIndexInName == 0 ? utils::capitalizeWord(value) : value);

    }
    ++parameterIndexInName;
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
