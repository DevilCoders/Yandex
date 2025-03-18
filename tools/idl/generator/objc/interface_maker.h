#pragma once

#include "common/doc_maker.h"
#include "common/import_maker.h"
#include "common/interface_maker.h"
#include "common/function_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

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

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Interface& i) override;

protected:
    ctemplate::TemplateDictionary* makeProperty(
            ctemplate::TemplateDictionary* parentDict,
            const FullScope& scope,
            const nodes::Property& p,
            bool isBaseItem = false) override;

private:
    ctemplate::TemplateDictionary* rootDict_;
};

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
