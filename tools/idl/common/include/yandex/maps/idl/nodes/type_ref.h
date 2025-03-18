#pragma once

#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/utils/exception.h>

#include <functional>
#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

enum class TypeId : std::size_t { // size_t is used when hashing (see below)
    /**
     * Only allowed as a return value of function that returns nothing.
     */
    Void,

    Bool,
    Int,
    Uint,
    Int64,
    Float,
    Double,

    String,

    TimeInterval,
    AbsTimestamp,
    RelTimestamp,

    Bytes,
    Color,

    Point,

    Bitmap,
    ImageProvider,
    AnimatedImageProvider,
    ModelProvider,
    AnimatedModelProvider,
    ViewProvider,

    Vector,
    Dictionary,

    /**
     * boost::any
     */
    Any,

    AnyCollection,

    PlatformView,

    /**
     * Any type declared manually (struct S{}, interface I{}, variant V{}...).
     * Check the name in TypeRef to find out the exact type.
     */
    Custom
};

std::ostream& operator<<(std::ostream& out, TypeId id);

struct TypeRef {
    TypeId id;

    /**
     * Type's qualified name (may be partially qualified). It is initialized
     * only for "TypeId::Custom" types - check id for built-in types.
     */
    std::optional<Scope> name;

    bool isConst;
    bool isOptional;

    /**
     * Generic type parameters. Allowed only vectors and dictionaries.
     */
    std::vector<TypeRef> parameters;
};

/**
 * Converts type reference into string form. Should not be used for code
 * generation - this method is for diagnostics only (e.g. syntax error
 * reporting).
 */
std::string typeRefToString(const TypeRef& typeRef);

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex

namespace std {

/**
 * Needed to be able to use TypeId as unordered_map key.
 */
template <>
struct hash<yandex::maps::idl::nodes::TypeId> {
    size_t operator()(const yandex::maps::idl::nodes::TypeId& typeId) const
    {
        return hash<size_t>()(static_cast<size_t>(typeId));
    }
};

} // namespace std
