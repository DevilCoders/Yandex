#pragma once

#include "common/import_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

class GuidRegistrationMaker : public nodes::Visitor {
public:
    GuidRegistrationMaker(
        ctemplate::TemplateDictionary* rootDict,
        const Idl* idl,
        common::ImportMaker* importMaker);

    virtual void onVisited(const nodes::Interface& i) override;
    virtual void onVisited(const nodes::Struct& s) override;

private:
    ctemplate::TemplateDictionary* rootDict_;

    const Idl* idl_;

    common::ImportMaker* importMaker_;

    Scope scope_;
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
