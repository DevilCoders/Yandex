#pragma once

#include <yandex/maps/idl/scope.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

/**
 * Unambiguously describes which message in which .proto file is given Struct,
 * Variant or Enum based on.
 */
struct ProtoMessage {
    /**
     * Path to the .proto file, e.g. "search/address.proto". All .proto-s will
     * have some root directory known to IDL parser and generators.
     */
    std::string pathToProto;

    /**
     * Holds "fully-qualified" protobuf message name. E.g. for
     * "Address.Component" in address.proto it will hold
     * {"Address", "Component"}.
     */
    Scope pathInProto;
};

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
