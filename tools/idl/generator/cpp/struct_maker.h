#pragma once

#include "common/doc_maker.h"
#include "common/enum_field_maker.h"
#include "common/import_maker.h"
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
namespace cpp {

class StructMaker : public common::StructMaker {
public:
    StructMaker(
        const common::PodDecider* podDecider,
        const common::TypeNameMaker* typeNameMaker,
        const common::EnumFieldMaker* enumFieldMaker,
        common::ImportMaker* importMaker,
        const common::DocMaker* docMaker,
        bool isHeader,
        bool ignoreCustomCodeLink = false);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Struct& s) override;

protected:
    virtual ctemplate::TemplateDictionary* makeField(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f) override;

private:
    const bool isHeader_;

    const bool ignoreCustomCodeLink_;
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
