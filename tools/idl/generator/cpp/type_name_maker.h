#pragma once

#include "common/type_name_maker.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/type_info.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

std::string fullName(const Scope& fullNameAsScope);
std::string fullName(const FullTypeRef& typeRef);

/**
 * Differs from full-name only by the absence of "::" prefix.
 */
std::string nativeName(const TypeInfo& typeInfo);

/**
 * Returns listener's binding's C++ type name.
 *
 * @param platform "android" for Android, and "ios" for iOS.
 */
std::string listenerBindingTypeName(
    const std::string& platform,
    const TypeInfo& typeInfo);

class TypeNameMaker : public common::TypeNameMaker {
public:
    TypeNameMaker() : common::TypeNameMaker(CPP, "::") { }

    virtual std::string make(const FullTypeRef& typeRef) const override
    {
        return make(typeRef, false);
    }
    virtual std::string makeRef(const FullTypeRef& typeRef) const override
    {
        return make(typeRef, true);
    }

private:
    std::string make(const FullTypeRef& typeRef, bool makeRef) const;
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
