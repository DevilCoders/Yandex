#pragma once

#include "common/function_maker.h"
#include "common/import_maker.h"
#include "common/type_name_maker.h"

#include "cpp/listener_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

class ListenerMaker : public cpp::ListenerMaker {
public:
    ListenerMaker(
        const common::TypeNameMaker* typeNameMaker,
        common::ImportMaker* importMaker,
        common::FunctionMaker* functionMaker,
        ctemplate::TemplateDictionary* rootDict);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Listener& l) override;

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Listener& l) const override;

private:
    ctemplate::TemplateDictionary* rootDict_;
};

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
