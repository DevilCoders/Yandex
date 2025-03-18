#include "common/function_maker.h"

#include "common/import_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/utils.h>
#include <yandex/maps/idl/utils/common.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

FunctionMaker::FunctionMaker(
    const PodDecider* podDecider,
    const TypeNameMaker* typeNameMaker,
    ImportMaker* importMaker,
    const std::string& targetLang,
    bool isHeader)
    : podDecider_(podDecider),
      typeNameMaker_(typeNameMaker),
      importMaker_(importMaker),
      targetLang_(targetLang),
      isHeader_(isHeader)
{
}

FunctionMaker::~FunctionMaker()
{
}

ctemplate::TemplateDictionary* FunctionMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Function& f,
    bool isLambda)
{
    auto dict = createDict(parentDict, scope, isLambda);

    dict->ShowSection(isHeader_ ? "IS_HEADER" : "IS_NOT_HEADER");
    if (isExcludeDoc(f.doc)) {
        dict->ShowSection("EXCLUDE");
    }

    // Name
    if (isLambda) {
        dict->SetValue("FUNCTION_NAME",
            utils::capitalizeWord(f.name[targetLang_].first()));
        dict->SetValue("LAMBDA_NAME",
            utils::capitalizeWord(f.name[targetLang_].first()));
    } else {
        dict->SetValue("FUNCTION_NAME", f.name[targetLang_].first());
    }

    // Result
    FullTypeRef resultTypeRef(scope, f.result.typeRef);
    importMaker_->addAll(resultTypeRef, false, isHeader_);

    dict->SetValue("RESULT_TYPE", typeNameMaker_->makeRef(resultTypeRef));
    if (f.result.typeRef.id != nodes::TypeId::Void) {
        dict->ShowSection("RETURNS_SOMETHING");
    }

    // Parameters
    for (parameterIndexInIdl_ = 0, parameterIndexInName_ = 0;
            parameterIndexInIdl_ < f.parameters.size();
            ++parameterIndexInIdl_) {
        makeParameter(dict, scope, f);
    }

    return dict;
}

ctemplate::TemplateDictionary* FunctionMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /*scope*/,
    bool isLambda) const
{
    return isLambda ? parentDict->AddSectionDictionary("METHOD") :
        tpl::addSectionedInclude(parentDict, "METHOD", "function.tpl");
}

void FunctionMaker::makeParameter(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Function& function)
{
    const auto& p = function.parameters[parameterIndexInIdl_];
    createParameterDict(parentDict, scope, p, function);
}

ctemplate::TemplateDictionary* FunctionMaker::createParameterDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::FunctionParameter& p,
    const nodes::Function& /* function */)
{
    FullTypeRef typeRef(scope, p.typeRef);
    importMaker_->addAll(typeRef, false, isHeader_);

    auto dict = parentDict->AddSectionDictionary("PARAM");

    dict->SetValue("PARAM_TYPE", typeNameMaker_->makeRef(typeRef));

    dict->SetValue("PARAM_NAME", p.name);

    if (p.defaultValue) {
        dict->SetValueAndShowSection(
            "VALUE", *p.defaultValue, "DEFAULT_VALUE");
    }

    dict->ShowSection(podDecider_->isPod(typeRef) ? "POD" : "NOT_POD");

    return dict;
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
