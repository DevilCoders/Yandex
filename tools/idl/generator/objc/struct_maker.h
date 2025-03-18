#pragma once

#include "common/doc_maker.h"
#include "common/enum_field_maker.h"
#include "common/pod_decider.h"
#include "common/struct_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

class StructMaker : public common::StructMaker {
public:
    StructMaker(
        const common::PodDecider* podDecider,
        const common::TypeNameMaker* typeNameMaker,
        const common::EnumFieldMaker* enumFieldMaker,
        common::ImportMaker* importMaker,
        const common::DocMaker* docMaker,
        ctemplate::TemplateDictionary* rootDict);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Struct& s) override;

protected:

    virtual std::string fieldValue(
        const FullScope& scope,
        const nodes::StructField& f) const override;

    virtual void fillFieldDict(
        ctemplate::TemplateDictionary* dict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f) override;

    virtual ctemplate::TemplateDictionary* makeField(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f) override;

    virtual bool addConstructorField(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::StructField& f,
        unsigned int fieldsNumber) override;

private:
    ctemplate::TemplateDictionary* rootDict_;
};

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
