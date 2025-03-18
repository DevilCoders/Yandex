#pragma once

#include "common/enum_field_maker.h"
#include "common/import_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"
#include "cpp/struct_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

class StructMaker : public cpp::StructMaker {
public:
    StructMaker(
        const common::PodDecider* podDecider,
        const common::TypeNameMaker* typeNameMaker,
        const common::EnumFieldMaker* enumFieldMaker,
        common::ImportMaker* importMaker,
        ctemplate::TemplateDictionary* rootDict,
        bool isHeader);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Struct& s) override;

    virtual ctemplate::TemplateDictionary* makeField(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f) override;

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Struct& s) const override;

    virtual void fillFieldDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f) override;

private:
    bool lastFillingVisibleField(
        const FullScope& scope,
        const nodes::StructField& f);

    ctemplate::TemplateDictionary* rootDict_;
    const bool isHeader_;
    std::size_t countFields_;
    std::size_t countNonInternalFields_;
    std::size_t numberFillingField_;

};

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
