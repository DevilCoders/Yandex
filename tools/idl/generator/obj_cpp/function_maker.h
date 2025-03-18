#pragma once

#include "cpp/function_maker.h"

#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

class FunctionMaker : public cpp::FunctionMaker {
public:
    using cpp::FunctionMaker::FunctionMaker;

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Function& f,
        bool isLambda) override;

    bool visitedStrongRefListenerParameter() const
    {
        return visitedStrongRefListenerParameter_;
    }

    bool visitedListenerReturnValue() const
    {
        return visitedListenerReturnValue_;
    }
    bool visitedStrongRefListenerReturnValue() const
    {
        return visitedStrongRefListenerReturnValue_;
    }

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        bool isLambda) const override;

    virtual void makeParameter(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Function& function) override;

    virtual ctemplate::TemplateDictionary* createParameterDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::FunctionParameter& p,
        const nodes::Function& function) override;

private:
    bool visitedStrongRefListenerParameter_{ false };

    bool visitedListenerReturnValue_{ false };
    bool visitedStrongRefListenerReturnValue_{ false };
};

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
