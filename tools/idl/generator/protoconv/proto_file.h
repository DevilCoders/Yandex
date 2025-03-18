#pragma once

#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/scope.h>

#include <google/protobuf/descriptor.h>

#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

/**
 * Holds given Idl node's corresponding Protobuf descriptor type.
 */
template <typename IdlNode> struct ProtoDescriptor;
template <> struct ProtoDescriptor<nodes::Enum>
{ using type = google::protobuf::EnumDescriptor; };
template <> struct ProtoDescriptor<nodes::Struct>
{ using type = google::protobuf::Descriptor; };
template <> struct ProtoDescriptor<nodes::Variant>
{ using type = google::protobuf::Descriptor; };

/**
 * Represents .proto file.
 *
 * TODO: 1) move to common/,
 *       2) make it very similar to Idl and Framework types (e.g. load it
 *          by Environment object, make it a simple struct...),
 *       3) wrap completely (make custom struct types for all those
 *          descriptors) so that protobuf API does not leak outside,
 *       4) remove global state (see ProtoFile c-tor), store it in the Env
 */
class ProtoFile {
public:
    /**
     * Parses .proto file. All objects use single static protobuf Importer, so
     * parsing is not thread safe.
     */
    ProtoFile(
        const std::string& rootDir,
        const std::string& basePackage,
        const std::string& relativePath);

    /**
     * Constructs the object by directly wrapping already parsed .proto file's
     * descriptor.
     */
    explicit ProtoFile(const google::protobuf::FileDescriptor* descriptor);

    bool operator==(const ProtoFile& other) const;
    bool operator!=(const ProtoFile& other) const;

    /**
     * Returns .proto file's package relative to base package of decode(...)
     * functions (yandex::maps::proto). It will be converted to "::"-separated
     * form - ending with "::" if not empty.
     */
    std::string relativeNamespacePrefix() const;

    /**
     * Searches for .proto type by its corresponding .idl node.
     */
    template <typename Node>
    const typename ProtoDescriptor<Node>::type* findProtoTypeDesc(
        const Node& n) const;

    /**
     * Returns field names of .proto Enum corresponding to given .idl Enum.
     */
    std::vector<std::string> enumFieldNames(const nodes::Enum& e) const;

private:
    std::string basePackage_;

    /**
     * Owned by protobuf's "importer", so no unique_ptr here.
     */
    const google::protobuf::FileDescriptor* descriptor_;
};

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
