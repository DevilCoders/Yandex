#pragma once

#include "common/import_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

class BindingTraitsMaker : public nodes::Visitor {
public:
    BindingTraitsMaker(
        ctemplate::TemplateDictionary* rootDict,
        const Idl* idl,
        common::ImportMaker* importMaker);

    virtual void onVisited(const nodes::Enum& e) override
    {
        if (!e.customCodeLink.baseHeader) {
            makeTraitsFor(e);
        }
    }
    virtual void onVisited(const nodes::Interface& i) override
    {
        if (!i.isStatic) {
            makeTraitsFor(i);
            traverseWithScope(scope_, i, this);
        }
    }
    virtual void onVisited(const nodes::Listener& l) override
    {
        if (!l.isLambda) {
            makeTraitsFor(l);
        }
    }
    virtual void onVisited(const nodes::Struct& s) override
    {
        if (!s.customCodeLink.baseHeader) {
            makeTraitsFor(s);
            traverseWithScope(scope_, s, this);
        }
    }
    virtual void onVisited(const nodes::Variant& v) override
    {
        if (!v.customCodeLink.baseHeader) {
            makeTraitsFor(v);
        }
    }

private:
    template <typename IdlNode>
    void makeTraitsFor(const IdlNode& n);

private:
    ctemplate::TemplateDictionary* rootDict_;
    ctemplate::TemplateDictionary* dict_{ nullptr };

    const Idl* idl_;

    common::ImportMaker* importMaker_;

    Scope scope_;
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
