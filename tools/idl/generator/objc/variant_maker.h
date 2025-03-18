#pragma once

#include "common/doc_maker.h"
#include "common/import_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"
#include "common/variant_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

class VariantMaker : public common::VariantMaker {
public:
    VariantMaker(
        const common::PodDecider* podDecider,
        const common::TypeNameMaker* typeNameMaker,
        common::ImportMaker* importMaker,
        const common::DocMaker* docMaker,
        ctemplate::TemplateDictionary* rootDict);

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

private:
    ctemplate::TemplateDictionary* rootDict_;
};

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
