#pragma once

#include <yandex/maps/idl/nodes/custom_code_link.h>
#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/protobuf.h>
#include <yandex/maps/idl/nodes/type_ref.h>

#include <optional>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

struct VariantField {
    TypeRef typeRef;
    std::string name;

    std::optional<std::string> protoField;
};

struct Variant {
    std::optional<Doc> doc;

    CustomCodeLink customCodeLink;

    Name name;

    std::optional<ProtoMessage> protoMessage;
    std::vector<VariantField> fields;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
