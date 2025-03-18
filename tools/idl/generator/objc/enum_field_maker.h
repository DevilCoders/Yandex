#pragma once

#include "common/enum_field_maker.h"

#include <yandex/maps/idl/full_type_ref.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

class EnumFieldMaker : public common::EnumFieldMaker {
public:
    using common::EnumFieldMaker::EnumFieldMaker;

    virtual std::string makeDefinition(
        const FullTypeRef& enumTypeRef,
        const std::string& fieldName) const override;

    virtual std::string makeReference(
        const FullTypeRef& enumTypeRef,
        const std::string& fieldName) const override;

    virtual std::string makeValue(
        const FullTypeRef& enumTypeRef,
        const std::string& fieldName,
        bool isLocalField) const override;
};

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
