#pragma once

#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/property.h>
#include <yandex/maps/idl/scope.h>

#include <optional>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

struct Interface {
    Interface() = default;
    Interface(Interface&&) = default;

    std::optional<Doc> doc;

    Name name;

    bool isVirtual;
    bool isViewDelegate;
    bool isStatic;

    enum class Ownership { Strong, Shared, Weak } ownership;

    /**
     * Base interface's qualified name.
     */
    std::optional<Scope> base;

    /**
     * Interfaces can have functions, properties, enums, structures,
     * variants and other interfaces.
     */
    Nodes nodes;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
