#pragma once

#include "common/function_maker.h"
#include "common/import_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

#include <cstddef>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

class FunctionMaker : public common::FunctionMaker {
public:
    FunctionMaker(
        const common::PodDecider* podDecider,
        const common::TypeNameMaker* typeNameMaker,
        common::ImportMaker* importMaker,
        bool isHeader);

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

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
