#pragma once

#include "common/variant_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/nodes/variant.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

class VariantMaker : public common::VariantMaker {
public:
    using common::VariantMaker::VariantMaker;

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Variant& v) override;

protected:
    virtual ctemplate::TemplateDictionary* makeField(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Variant& v,
        const nodes::VariantField& f,
        int fieldIndex) const override;
};

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
