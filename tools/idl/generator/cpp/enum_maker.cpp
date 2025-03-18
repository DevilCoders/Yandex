#include "cpp/enum_maker.h"

#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

EnumMaker::EnumMaker(
    const common::TypeNameMaker* typeNameMaker,
    const common::EnumFieldMaker* fieldMaker,
    const common::DocMaker* docMaker,
    common::ImportMaker* importMaker)
    : common::EnumMaker(typeNameMaker, fieldMaker, docMaker),
      importMaker_(importMaker)
{
}

ctemplate::TemplateDictionary* EnumMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Enum& e)
{
    if (e.customCodeLink.baseHeader) {
        importMaker_->addImportPath('<' + *e.customCodeLink.baseHeader + '>');
        return nullptr;
    }

    return common::EnumMaker::make(parentDict, scope, e);
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
