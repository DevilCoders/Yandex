#pragma once

#include "common/common.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

#include <cstdint>
#include <cstddef>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

class ImportMaker;
class PodDecider;
class TypeNameMaker;

class FunctionMaker {
public:
    FunctionMaker(
        const PodDecider* podDecider,
        const TypeNameMaker* typeNameMaker,
        ImportMaker* importMaker,
        const std::string& targetlang,
        bool isHeader);

    virtual ~FunctionMaker();

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Function& f,
        bool isLambda);

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        bool isLambda) const;

    virtual void makeParameter(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Function& function);

    /**
     * Creates and fills parameter dictionary.
     */
    virtual ctemplate::TemplateDictionary* createParameterDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::FunctionParameter& p,
        const nodes::Function& function);

protected:
    const PodDecider* podDecider_;
    const TypeNameMaker* typeNameMaker_;
    ImportMaker* importMaker_;

    const std::string targetLang_;

    const bool isHeader_;

    std::size_t parameterIndexInIdl_ = 0;
    // Index in platform function name. It may differ from previous index
    // in case param is lambda listener.
    std::size_t parameterIndexInName_ = 0;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
