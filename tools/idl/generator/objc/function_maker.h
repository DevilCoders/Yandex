#pragma once

#include "common/function_maker.h"

#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/nodes/type_ref.h>

#include <ctemplate/template.h>

#include <cstdint>
#include <cstddef>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

class FunctionMaker : public common::FunctionMaker {
public:
    using common::FunctionMaker::FunctionMaker;

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Function& f,
        bool isLambda) override;

protected:
    virtual void makeParameter(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Function& function) override;

    virtual ctemplate::TemplateDictionary* createParameterDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::FunctionParameter& p,
        const nodes::Function& function) override;
};

void setObjCMethodNameParamPart(
    ctemplate::TemplateDictionary* dict,
    const std::string& value,
    const nodes::Function& function,
    std::size_t& parameterIndexInName);

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
