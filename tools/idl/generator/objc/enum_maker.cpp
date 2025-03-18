#include "objc/enum_maker.h"

#include "common/common.h"
#include <yandex/maps/idl/utils.h>
#include "tpl/tpl.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

EnumMaker::EnumMaker(
    const common::TypeNameMaker* typeNameMaker,
    const common::EnumFieldMaker* fieldMaker,
    const common::DocMaker* docMaker)
    : common::EnumMaker(typeNameMaker, fieldMaker, docMaker)
{
}

ctemplate::TemplateDictionary* EnumMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Enum& e)
{
    if (!needGenerateNode(scope, e))
        return parentDict;
    return common::EnumMaker::make(parentDict, scope, e);
}

ctemplate::TemplateDictionary* EnumMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /*scope*/) const
{
    return tpl::addSectionedInclude(parentDict, "NON_FWD_DECL_ABLE_TYPE", "enum.tpl");
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
