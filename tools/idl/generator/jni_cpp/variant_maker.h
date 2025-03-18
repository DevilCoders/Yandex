#pragma once

#include "common/variant_maker.h"

#include <yandex/maps/idl/idl.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

class VariantMaker : public common::VariantMaker {
public:
    VariantMaker(
        const common::PodDecider* podDecider,
        const common::TypeNameMaker* typeNameMaker,
        common::ImportMaker* importMaker,
        ctemplate::TemplateDictionary* rootDict);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Variant& v) override;

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope) const override;

    virtual ctemplate::TemplateDictionary* makeField(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Variant& v,
        const nodes::VariantField& f,
        int fieldIndex) const override;

private:
    ctemplate::TemplateDictionary* rootDict_;
};

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
