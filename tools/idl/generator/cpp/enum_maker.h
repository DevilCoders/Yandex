#pragma once

#include "common/doc_maker.h"
#include "common/enum_field_maker.h"
#include "common/enum_maker.h"
#include "common/import_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

class EnumMaker : public common::EnumMaker {
public:
    EnumMaker(
        const common::TypeNameMaker* typeNameMaker,
        const common::EnumFieldMaker* fieldMaker,
        const common::DocMaker* docMaker,
        common::ImportMaker* importMaker);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Enum& e) override;

private:
    common::ImportMaker* importMaker_;
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
