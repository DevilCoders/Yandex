#include "cpp/bitfield_operators.h"

#include "tpl/tpl.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

BitfieldOpsEnumMaker::BitfieldOpsEnumMaker(
    const common::TypeNameMaker* typeNameMaker,
    ctemplate::TemplateDictionary* rootDict)
    : common::EnumMaker(typeNameMaker, nullptr, nullptr),
      rootDict_(rootDict)
{
}

ctemplate::TemplateDictionary* BitfieldOpsEnumMaker::make(
    ctemplate::TemplateDictionary* /* parentDict */,
    FullScope scope,
    const nodes::Enum& e)
{
    if (e.isBitField && !e.customCodeLink.baseHeader) {
        if (!dict_) {
            dict_ = tpl::addInclude(rootDict_, "BITFIELD_OPERATORS", "bitfield_operators.tpl");
        }

        auto operatorsDict = dict_->AddSectionDictionary("OPERATOR");
        auto scopedName = typeNameMaker_->make(FullTypeRef(scope, e.name));
        operatorsDict->SetValue("ENUM_SCOPED_NAME", scopedName);
    }

    return dict_;
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
