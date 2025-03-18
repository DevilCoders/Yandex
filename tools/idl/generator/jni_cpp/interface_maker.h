#pragma once

#include "common/function_maker.h"
#include "common/import_maker.h"
#include "common/interface_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"
#include "cpp/interface_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

class InterfaceMaker : public cpp::InterfaceMaker {
public:
    InterfaceMaker(
        const common::PodDecider* podDecider,
        const common::TypeNameMaker* typeNameMaker,
        common::ImportMaker* importMaker,
        common::FunctionMaker* functionMaker,
        bool isHeader,
        ctemplate::TemplateDictionary* rootDict);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Interface& i) override;

    bool generatedStrongRefListenerProperty() const
    {
        return generatedStrongRefListenerProperty_;
    }

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Interface& i) const override;

    virtual ctemplate::TemplateDictionary* makeFunction(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Interface& i,
        const nodes::Function& f,
        bool isBaseItem) override;

    virtual ctemplate::TemplateDictionary* makeProperty(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Property& p,
        bool isBaseItem) override;

private:
    ctemplate::TemplateDictionary* rootDict_;

    bool generatedStrongRefListenerProperty_;
};

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
