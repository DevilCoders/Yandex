#pragma once

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/type_info.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

class TypeNameMaker {
public:
    TypeNameMaker(
        const std::string& targetLang,
        const std::string& delimiter);

    virtual ~TypeNameMaker() { }

    virtual std::string makeConstructorName(const TypeInfo& typeInfo) const;
    virtual std::string makeInstanceName(const TypeInfo& typeInfo) const;

    /**
     * Makes given typeRef's full name.
     */
    virtual std::string make(const FullTypeRef& typeRef) const = 0;

    /**
     * Makes given typeRef's full "reference name".
     *
     * E.g. shared_ref interface's reference name can be
     *     'const std::shared_ptr<ns::scope::I>&',
     * while its type name will be just
     *     'ns::scope::I'.
     */
    virtual std::string makeRef(const FullTypeRef& typeRef) const = 0;

protected:
    const std::string targetLang_;

    /**
     * Delimiter between scope parts.
     */
    const std::string delimiter_;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
