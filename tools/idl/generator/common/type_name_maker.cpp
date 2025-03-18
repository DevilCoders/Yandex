#include "common/type_name_maker.h"

#include <yandex/maps/idl/utils/common.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

TypeNameMaker::TypeNameMaker(
    const std::string& targetLang,
    const std::string& delimiter)
    : targetLang_(targetLang),
      delimiter_(delimiter)
{
}

std::string TypeNameMaker::makeConstructorName(
    const TypeInfo& typeInfo) const
{
    return typeInfo.name[targetLang_];
}

std::string TypeNameMaker::makeInstanceName(const TypeInfo& typeInfo) const
{
    return utils::unCapitalizeWord(makeConstructorName(typeInfo));
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
