#pragma once

#include <yandex/maps/idl/customizable.h>
#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/scope.h>

#include <optional>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

/**
 * Holds Idl function's name, with possible target-specific overrides.
 */
using FunctionName = CustomizableValue<Scope>;

struct FunctionResult {
    TypeRef typeRef;
};

struct FunctionParameter {
    TypeRef typeRef;

    std::string name;

    /**
     * The expression (in text form) of parameter's default value.
     */
    std::optional<std::string> defaultValue;
};

struct Function {
    std::optional<Doc> doc;

    FunctionName name;

    FunctionResult result;
    std::vector<FunctionParameter> parameters;

    bool isConst;

    enum class ThreadRestriction { Ui, Bg, None } threadRestriction;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
