#pragma once

#include "common/enum_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

/**
 * Generates operators |, |=, & and &= for bit-field enums.
 */
class BitfieldOpsEnumMaker : public common::EnumMaker {
public:
    BitfieldOpsEnumMaker(
        const common::TypeNameMaker* typeNameMaker,
        ctemplate::TemplateDictionary* rootDict);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Enum& e) override;

private:
    ctemplate::TemplateDictionary* rootDict_;

    ctemplate::TemplateDictionary* dict_{ nullptr };
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
