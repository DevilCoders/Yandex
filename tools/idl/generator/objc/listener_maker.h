#pragma once

#include "common/doc_maker.h"
#include "common/function_maker.h"
#include "common/listener_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

class ListenerMaker : public common::ListenerMaker {
public:
    ListenerMaker(
        const common::TypeNameMaker* typeNameMaker,
        const common::DocMaker* docMaker,
        common::FunctionMaker* functionMaker);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Listener& l) override;

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Listener& l) const override;

    virtual ctemplate::TemplateDictionary* makeFunction(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Listener& l,
        const nodes::Function& f) override;
};

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
