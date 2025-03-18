#pragma once

#include <yandex/maps/idl/customizable.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/scope.h>

#include <functional>
#include <string>
#include <variant>

namespace yandex {
namespace maps {
namespace idl {

using Type = std::variant<
    const nodes::Enum*,
    const nodes::Interface*,
    const nodes::Listener*,
    const nodes::Struct*,
    const nodes::Variant*>;

struct Idl;
struct TypeInfo {
    /**
     * File where this type is declared.
     */
    const Idl* idl;

    /**
     * File-level scope (for each target language) where this type is
     * declared.
     */
    CustomizableValue<Scope> scope;

    /**
     * Name of this type.
     */
    nodes::Name name;

    /**
     * The type itself.
     */
    Type type;

    /**
     * Fully-qualified name (for each target language) of this type as Scope
     * object.
     */
    CustomizableValue<Scope> fullNameAsScope;

    /**
     * Itself or enclosing type has @internal tag
     */
    bool isInternal;

    /**
     * Fully-qualified Idl name of this type.
     */
    operator std::string() const
    {
        return fullNameAsScope.original();
    }
};

inline bool operator==(const TypeInfo& left, const TypeInfo& right)
{
    // TypeInfo is uniquely identified by its Type, and Type is a variant of
    // pointers to objects from the syntax tree. Those objects are never
    // copied - at least they MUST never be copied! So pointer comparison is
    // sufficient:
    return left.type == right.type;
}

inline bool operator!=(const TypeInfo& left, const TypeInfo& right)
{
    return !(left == right);
}

inline bool operator<(const TypeInfo& left, const TypeInfo& right)
{
    return (std::string)left < (std::string)right;
}

} // namespace idl
} // namespace maps
} // namespace yandex

namespace std {

template <>
struct hash<yandex::maps::idl::TypeInfo> {
    size_t operator()(const yandex::maps::idl::TypeInfo& info) const
    {
        return std::hash<std::string>()(info);
    }
};

} // namespace std
