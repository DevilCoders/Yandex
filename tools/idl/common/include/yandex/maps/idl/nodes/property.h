#pragma once

#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/type_ref.h>

#include <optional>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

struct Property {
    std::optional<Doc> doc;

    /**
     * Is generated inside the getter.
     */
    bool isGenerated;

    TypeRef typeRef;

    std::string name;

    bool isReadonly;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
