#pragma once

#include "common/type_name_maker.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/targets.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

class TypeNameMaker : public common::TypeNameMaker {
public:
    TypeNameMaker() : common::TypeNameMaker(JAVA, ".") { }

    virtual std::string make(const FullTypeRef& typeRef) const override;
    virtual std::string makeRef(const FullTypeRef& typeRef) const override;
};

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
