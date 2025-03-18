#include "objc/enum_field_maker.h"

#include "common/type_name_maker.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

std::string EnumFieldMaker::makeDefinition(
    const FullTypeRef& enumTypeRef,
    const std::string& fieldName) const
{
    return typeNameMaker_->make(enumTypeRef) + fieldName;
}

std::string EnumFieldMaker::makeReference(
    const FullTypeRef& enumTypeRef,
    const std::string& fieldName) const
{
    return typeNameMaker_->make(enumTypeRef) + fieldName;
}

std::string EnumFieldMaker::makeValue(
    const FullTypeRef& enumTypeRef,
    const std::string& fieldName,
    bool /* isLocalField */) const
{
    return typeNameMaker_->make(enumTypeRef) + fieldName;
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
