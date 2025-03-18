#pragma once

#include "common/struct_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

class StructMaker : public common::StructMaker {
public:
    using common::StructMaker::StructMaker;

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Struct& s) override;

    virtual ctemplate::TemplateDictionary* makeField(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f) override;

    virtual void fillFieldDict(
        ctemplate::TemplateDictionary* dict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f) override;

protected:

    virtual bool addConstructorField(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::StructField& f,
        unsigned int fieldsNumber) override;
};

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
