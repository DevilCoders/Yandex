#include "cpp/function_maker.h"

#include "common/common.h"
#include "cpp/type_name_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils/common.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

FunctionMaker::FunctionMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    bool isHeader)
    : common::FunctionMaker(
          podDecider, typeNameMaker, importMaker, "", isHeader)
{
}

namespace {

std::string lambdaScopePrefix(
    const FullScope& /* scope */,
    const TypeInfo& typeInfo)
{
    auto prefix = fullName(typeInfo.fullNameAsScope[CPP]);

    auto lastDelimiterPosition = prefix.find_last_of(':');
    if (lastDelimiterPosition == std::string::npos) {
        lastDelimiterPosition = 0;
    }
    if (lastDelimiterPosition > 0) {
        REQUIRE(prefix[lastDelimiterPosition - 1] == ':',
            "Single colon cannot be a C++ type delimiter");

        lastDelimiterPosition--;
    }

    prefix = prefix.substr(0, lastDelimiterPosition);
    if (!prefix.empty()) {
        prefix += "::";
    }

    return prefix;
}

} // namespace

void FunctionMaker::makeParameter(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Function& function)
{
    const auto& p = function.parameters[parameterIndexInIdl_];
    if (p.typeRef.id == nodes::TypeId::Custom) {
        FullTypeRef typeRef(scope, p.typeRef);

        if (typeRef.isLambdaListener()) {
            importMaker_->addAll(typeRef, false);

            auto lambdaPrefix = lambdaScopePrefix(scope, typeRef.info());
            for (const auto& f : typeRef.as<nodes::Listener>()->functions) {
                auto paramDict = parentDict->AddSectionDictionary("PARAM");

                auto lambdaName = lambdaPrefix +
                    utils::capitalizeWord(f.name[targetLang_].first());
                paramDict->SetValue("PARAM_TYPE", lambdaName);

                if (p.defaultValue) {
                    paramDict->SetValueAndShowSection(
                        "VALUE", *p.defaultValue, "DEFAULT_VALUE");
                }
                paramDict->SetValue(
                    "PARAM_NAME", f.name[targetLang_].first());

                paramDict->ShowSection("CONST_PARAM");
                paramDict->ShowSection("REF_PARAM");
            }

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
    if (!typeRef.isWeakInterface() && typeRef.isOptional() || typeRef.isByReferenceInCpp()) {
        dict->ShowSection("REF_PARAM");
    }
    if (typeRef.isBridged() && !typeRef.isConst()) {
        dict->ShowSection("CONST_PARAM");
    }

    return dict;
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
