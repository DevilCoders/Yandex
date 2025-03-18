#pragma once

#include <yandex/maps/idl/nodes/custom_code_link.h>
#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/protobuf.h>

#include <optional>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

struct EnumField {
    std::optional<Doc> doc;

    std::string name;

    /**
     * The expression (in text form) of enum field's value.
     *
     * In our .idl syntax either enum's all fields have values or none, so
     * generators don't need to worry about constructing "previous field value
     * increment expression" for next field values.
     */
    std::optional<std::string> value;
};

struct Enum {
    std::optional<Doc> doc;

    CustomCodeLink customCodeLink;

    bool isBitField;

    Name name;

    std::optional<ProtoMessage> protoMessage;

    std::vector<EnumField> fields;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
