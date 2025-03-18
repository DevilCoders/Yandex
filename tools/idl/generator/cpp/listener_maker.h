#pragma once

#include "common/doc_maker.h"
#include "common/function_maker.h"
#include "common/import_maker.h"
#include "common/listener_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

class ListenerMaker : public common::ListenerMaker {
public:
    ListenerMaker(
        const common::TypeNameMaker* typeNameMaker,
        common::ImportMaker* importMaker,
        const common::DocMaker* docMaker,
        common::FunctionMaker* functionMaker);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Listener& l) override;

protected:
    common::ImportMaker* importMaker_;
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
