#pragma once

#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/scope.h>

#include <optional>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

struct Listener {
    std::optional<Doc> doc;

    /**
     * Some listeners should be turned into separate call-backs, if possible
     * in target language.
     */
    bool isLambda;

    /**
     * Is owned by native code.
     */
    bool isStrongRef;

    Name name;

    /**
     * Base listener's qualified name.
     */
    std::optional<Scope> base;

    std::vector<Function> functions;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
