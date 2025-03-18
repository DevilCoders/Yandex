#pragma once

#include "cpp/function_maker.h"

#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

class FunctionMaker : public cpp::FunctionMaker {
public:
    using cpp::FunctionMaker::FunctionMaker;

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Function& f,
        bool isLambda) override;

    bool generatedStrongRefListenerParameter() const
    {
        return generatedStrongRefListenerParameter_;
    }

    bool generatedListenerReturnValue() const
    {
        return generatedListenerReturnValue_;
    }
    bool generatedStrongRefListenerReturnValue() const
    {
        return generatedStrongRefListenerReturnValue_;
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

private:
    bool generatedStrongRefListenerParameter_{ false };

    bool generatedListenerReturnValue_{ false };
    bool generatedStrongRefListenerReturnValue_{ false };
};

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
