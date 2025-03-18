#pragma once

#include <yandex/maps/idl/nodes/custom_code_link.h>
#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/protobuf.h>
#include <yandex/maps/idl/nodes/type_ref.h>

#include <optional>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

enum class StructKind {
    Bridged,
    Lite,
    Options
};

/**
 * Field declaration inside of a structure.
 */
struct StructField {
    std::optional<Doc> doc;

    TypeRef typeRef;
    std::string name;

    /**
     * The expression (in text form) of struct field's default value.
     */
    std::optional<std::string> defaultValue;

    std::optional<std::string> protoField;
};

struct Struct {
    Struct() = default;
    Struct(Struct&&) = default;

    std::optional<Doc> doc;

    CustomCodeLink customCodeLink;

    StructKind kind;

    Name name;

    std::optional<ProtoMessage> protoMessage;

    /**
     * Structures can have fields, enums, variants and other structures.
     */
    Nodes nodes;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
