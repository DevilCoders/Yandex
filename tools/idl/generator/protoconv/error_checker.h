#pragma once

#include "protoconv/proto_file.h"

#include <yandex/maps/idl/utils/exception.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/protobuf.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/type_ref.h>
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

class ErrorChecker : public nodes::Visitor {
public:
    ErrorChecker(const Idl* idl);

    std::vector<std::string> check();

    using nodes::Visitor::onVisited;

    virtual void onVisited(const nodes::Enum& e) override;
    virtual void onVisited(const nodes::Interface& i) override;
    virtual void onVisited(const nodes::Struct& s) override;
    virtual void onVisited(const nodes::Variant& v) override;

private:
    /**
     * Adds an error.
     *
     * @param context - more info about where the error was found
     * @param message - error message
     */
    void addError(
        const std::string& context,
        const std::string& message);

    template <typename Node>
    void addProtoTypeNotFoundError(const Node& n)
    {
        addError(n.name.original(), "Protobuf type not found '" +
            n.protoMessage->pathInProto.asString(".") + "'");
    }

    /**
     * Adds an error telling that .idl field's type is not based on .proto
     * field's type.
     */
    void addIncompatibleTypesError(const std::string& fieldName);

    /**
     * Checks given node.
     */
    template <typename Node>
    void visit(const Node& n)
    {
        if (n.protoMessage) {
            if (n.customCodeLink.protoconvHeader) {
                return;
            }

            try {
                ProtoFile protoFile(
                    idl_->env->config.inProtoRoot,
                    idl_->env->config.baseProtoPackage,
                    n.protoMessage->pathToProto);
                checkNodeCompatibility(protoFile, n);
            } catch(const utils::Exception& e) {
                addError(n.name.original(), std::string(e.what()));
            }
        }
    }

    /**
     * Checks if given .proto file has compatible .idl enum.
     */
    void checkNodeCompatibility(
        const ProtoFile& protoFile,
        const nodes::Enum& e);

    /**
     * Checks if given .proto file has compatible .idl struct.
     */
    void checkNodeCompatibility(
        const ProtoFile& protoFile,
        const nodes::Struct& s);

    /**
     * Checks if given .proto file has compatible .idl variant.
     */
    void checkNodeCompatibility(
        const ProtoFile& protoFile,
        const nodes::Variant& v);

    /**
     * Checks whether field descriptor's type is convertible into typeRef.
     */
    void checkFieldCompatibility(
        const std::string& fieldName,
        const nodes::TypeRef& typeRef,
        const google::protobuf::FieldDescriptor* fieldDesc,
        bool insideVector = false);

    /**
     * Checks if .idl field of enum type is compatible with .proto enum.
     */
    void checkEnumFieldCompatibility(
        const std::string& fieldName,
        const Type& type,
        const google::protobuf::EnumDescriptor* protoEnumDesc);

    /**
     * Checks if .idl field of struct or variant type is compatible with
     * .proto message.
     */
    void checkMessageFieldCompatibility(
        const std::string& fieldName,
        const Type& type,
        const google::protobuf::Descriptor* protoTypeDesc);

    /**
     * Checks if .idl field's type references correct .proto type.
     */
    template <typename Node>
    void checkFieldProtoReference(
        const std::string& fieldName,
        const Node& fieldTypeNode,
        const typename ProtoDescriptor<Node>::type* protoTypeDesc);

private:
    const Idl* idl_;

    Scope scope_;

    std::vector<std::string> errors_;
};

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
