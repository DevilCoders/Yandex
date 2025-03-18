#pragma once

#include "common/doc_maker.h"
#include "common/enum_maker.h"
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
namespace obj_cpp {

class StructMaker : public common::StructMaker {
public:
    StructMaker(
        const common::PodDecider* podDecider,
        const common::TypeNameMaker* typeNameMaker,
        const common::EnumFieldMaker* enumFieldMaker,
        common::ImportMaker* importMaker,
        const common::DocMaker* docMaker,
        ctemplate::TemplateDictionary* rootDict,
        bool isHeader);

    bool visited() const { return visited_; }

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Struct& s) override;

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Struct& s) const override;

    virtual void fillFieldDict(
        ctemplate::TemplateDictionary* dict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f) override;

private:
    ctemplate::TemplateDictionary* rootDict_;
    bool isHeader_;

    bool visited_;

    size_t fieldNumber_;
};

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
