#pragma once

#include "common/doc_maker.h"
#include "common/function_maker.h"
#include "common/import_maker.h"
#include "common/interface_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

class InterfaceMaker : public common::InterfaceMaker {
public:
    InterfaceMaker(
        const common::PodDecider* podDecider,
        const common::TypeNameMaker* typeNameMaker,
        common::ImportMaker* importMaker,
        const common::DocMaker* docMaker,
        common::FunctionMaker* functionMaker,
        bool isHeader,
        ctemplate::TemplateDictionary* rootDict);

    bool visited() const { return visited_; }
    bool visitedNotStatic() const { return visitedNotStatic_; }
    bool visitedStrongRefListenerProperty() const
    {
        return visitedStrongRefListenerProperty_;
    }

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Interface& i) override;


protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Interface& i) const override;

    virtual ctemplate::TemplateDictionary* makeProperty(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Property& p,
        bool isBaseItem) override;

private:
    ctemplate::TemplateDictionary* rootDict_;

    bool visited_;
    bool visitedNotStatic_;
    bool visitedStrongRefListenerProperty_;
};

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
