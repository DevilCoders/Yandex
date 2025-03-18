#include "java/enum_field_maker.h"

#include "common/type_name_maker.h"

#include <yandex/maps/idl/utils/common.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

namespace {

std::string javaFieldName(std::string idlFieldName)
{
    return utils::toUpperCase(idlFieldName);
}

} // namespace

std::string EnumFieldMaker::makeDefinition(
    const FullTypeRef& /* enumTypeRef */,
    const std::string& fieldName) const
{
    return javaFieldName(fieldName);
}

std::string EnumFieldMaker::makeReference(
    const FullTypeRef& /* enumTypeRef */,
    const std::string& fieldName) const
{
    return javaFieldName(fieldName);
}

std::string EnumFieldMaker::makeValue(
    const FullTypeRef& enumTypeRef,
    const std::string& fieldName,
    bool isLocalField) const
{
    auto value = javaFieldName(fieldName) +
        (enumTypeRef.isBitfieldEnum() ? ".value" : "");
    if (!isLocalField) {
        value = typeNameMaker_->make(enumTypeRef) + '.' + value;
    }
    return value;
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
