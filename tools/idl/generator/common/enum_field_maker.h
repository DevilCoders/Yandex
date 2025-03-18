#pragma once

#include <yandex/maps/idl/full_type_ref.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

class TypeNameMaker;

class EnumFieldMaker {
public:
    EnumFieldMaker(const TypeNameMaker* typeNameMaker);

    virtual ~EnumFieldMaker();

    virtual std::string makeDefinition(
        const FullTypeRef& enumTypeRef,
        const std::string& fieldName) const = 0;

    virtual std::string makeReference(
        const FullTypeRef& enumTypeRef,
        const std::string& fieldName) const = 0;

    virtual std::string makeValue(
        const FullTypeRef& enumTypeRef,
        const std::string& fieldName,
        bool isLocalField) const = 0;

protected:
    const TypeNameMaker* typeNameMaker_;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
