#pragma once

#include "common/doc_maker.h"
#include "common/function_maker.h"
#include "common/import_maker.h"
#include "common/listener_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/listener.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

class ListenerMaker : public common::ListenerMaker {
public:
    ListenerMaker(
        const common::TypeNameMaker* typeNameMaker,
        common::ImportMaker* importMaker,
        const common::DocMaker* docMaker,
        common::FunctionMaker* functionMaker);

protected:
    virtual ctemplate::TemplateDictionary* makeFunction(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Listener& l,
        const nodes::Function& f) override;

    common::ImportMaker* importMaker_;
};

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
