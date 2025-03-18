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
namespace objc {

class TypeNameMaker : public common::TypeNameMaker {
public:
    TypeNameMaker() : common::TypeNameMaker(OBJC, "") { }

    virtual std::string makeConstructorName(
        const TypeInfo& typeInfo) const override;
    virtual std::string makeInstanceName(
        const TypeInfo& typeInfo) const override;

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

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
