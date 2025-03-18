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

class ImportMaker;

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

class ListenerMaker : public common::ListenerMaker {
public:
    ListenerMaker(
        const common::TypeNameMaker* typeNameMaker,
        const common::DocMaker* docMaker,
        common::FunctionMaker* functionMaker,
        ctemplate::TemplateDictionary* rootDict,
        bool isHeader,
        common::ImportMaker* importMaker);

    bool visited() const { return visited_; }

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

private:
    ctemplate::TemplateDictionary* rootDict_;
    bool isHeader_;

    common::ImportMaker* importMaker_;

    bool visited_;
};

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
