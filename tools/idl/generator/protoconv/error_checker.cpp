#include "protoconv/error_checker.h"

#include "common/common.h"

#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/nodes/root.h>

#include <boost/algorithm/string.hpp>

#include <set>
#include <unordered_map>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

namespace {

namespace gp = google::protobuf;

const std::unordered_map<unsigned int, nodes::TypeId> DESCRIPTOR_TO_POD {
    { gp::FieldDescriptor::TYPE_BOOL,     nodes::TypeId::Bool   },
    { gp::FieldDescriptor::TYPE_INT32,    nodes::TypeId::Int    },
    { gp::FieldDescriptor::TYPE_SINT32,   nodes::TypeId::Int    },
    { gp::FieldDescriptor::TYPE_SFIXED32, nodes::TypeId::Int    },
    { gp::FieldDescriptor::TYPE_UINT32,   nodes::TypeId::Uint   },
    { gp::FieldDescriptor::TYPE_FIXED32,  nodes::TypeId::Uint   },
    { gp::FieldDescriptor::TYPE_INT64,    nodes::TypeId::Int64  },
    { gp::FieldDescriptor::TYPE_SINT64,   nodes::TypeId::Int64  },
    { gp::FieldDescriptor::TYPE_SFIXED64, nodes::TypeId::Int64  },
    { gp::FieldDescriptor::TYPE_UINT64,   nodes::TypeId::Int64  },
    { gp::FieldDescriptor::TYPE_FIXED64,  nodes::TypeId::Int64  },
    { gp::FieldDescriptor::TYPE_FLOAT,    nodes::TypeId::Float  },
    { gp::FieldDescriptor::TYPE_DOUBLE,   nodes::TypeId::Double },
    { gp::FieldDescriptor::TYPE_STRING,   nodes::TypeId::String }
};

} // namespace

ErrorChecker::ErrorChecker(const Idl* idl)
    : idl_(idl)
{
}

std::vector<std::string> ErrorChecker::check()
{
    errors_.clear();
    idl_->root.nodes.traverse(this);
    return errors_;
}

void ErrorChecker::onVisited(const nodes::Enum& e)
{
    visit(e);
}

void ErrorChecker::onVisited(const nodes::Interface& i)
{
    traverseWithScope(scope_, i, this);
}

void ErrorChecker::onVisited(const nodes::Struct& s)
{
    visit(s);
    traverseWithScope(scope_, s, this);
}

void ErrorChecker::onVisited(const nodes::Variant& v)
{
    visit(v);
}

void ErrorChecker::addError(
    const std::string& context,
    const std::string& message)
{
    std::string scopeString = scope_;
    if (!context.empty()) {
        if (!scopeString.empty()) {
            scopeString += '.';
        }
        scopeString += context;
    }

    errors_.push_back(scopeString + ": " + message);
}

void ErrorChecker::addIncompatibleTypesError(const std::string& fieldName)
{
    addError(fieldName,
        "Idl field's type is not based on Protobuf field's type");
}

void ErrorChecker::checkNodeCompatibility(
    const ProtoFile& protoFile,
    const nodes::Enum& e)
{
    const auto* descriptor = protoFile.findProtoTypeDesc(e);
    if (descriptor == nullptr) {
        addProtoTypeNotFoundError(e);
        return;
    }

    // .idl enum must "cover" all .proto enum's constants!

    // Gather names of all constants in .proto enum.
    std::set<std::string> protoConstantNames;
    int numberOfProtoConstants = descriptor->value_count();
    for (int index = 0; index < numberOfProtoConstants; ++index) {
        protoConstantNames.insert(descriptor->value(index)->name());
    }

    // Now, remove all that are "covered" by .idl enum
    for (const nodes::EnumField& field : e.fields) {
        protoConstantNames.erase(utils::toUpperCase(field.name));
    }

    // And finally, check if some constants were left "uncovered"
    if (!protoConstantNames.empty()) {
        addError(e.name.original(), "Idl enum doesn't have constants [" +
            boost::algorithm::join(protoConstantNames, ", ") +
            "] from Protobuf enum. Please check that in .idl, your enum "
            "constants are in CamelCase, and in .proto - UPPER_CASE.");
    }
}

void ErrorChecker::checkNodeCompatibility(
    const ProtoFile& protoFile,
    const nodes::Struct& s)
{
    const auto* descriptor = protoFile.findProtoTypeDesc(s);
    if (descriptor == nullptr) {
        addProtoTypeNotFoundError(s);
        return;
    }

    lambdaTraverseWithScope(scope_, s,
        [&](const nodes::StructField& f)
        {
            if (!f.protoField) {
                return; // No need to check this non-Protobuf-based field
            }

            const std::string& protoField = *f.protoField;
            const gp::FieldDescriptor* fd =
                descriptor->FindFieldByName(protoField.c_str());
            if (fd == nullptr) {
                addError(f.name, "Field not found in Protobuf message");
                return;
            }

            if (fd->is_required() && f.typeRef.isOptional) {
                addError(f.name,
                    "Field is 'optional', but in Protobuf it is 'required'");
            } else if (fd->is_optional() && !fd->has_default_value() && !f.typeRef.isOptional) {
                addError(f.name,
                    "Field is 'required', but in Protobuf it is 'optional' without default value");
            }

            checkFieldCompatibility(f.name, f.typeRef, fd);
        });
}

void ErrorChecker::checkNodeCompatibility(
    const ProtoFile& protoFile,
    const nodes::Variant& v)
{
    const auto* descriptor = protoFile.findProtoTypeDesc(v);
    if (descriptor == nullptr) {
        addProtoTypeNotFoundError(v);
        return;
    }

    ScopeGuard guard(&scope_, v.name.original());

    // .idl variant must "cover" all .proto variant's fields!

    // Insert names of all .proto variant fields
    std::set<std::string> protoVariantFieldNames;
    int numberOfProtoFields = descriptor->field_count();
    for (int index = 0; index < numberOfProtoFields; ++index) {
        protoVariantFieldNames.insert(descriptor->field(index)->name());
    }

    // Now, remove those that are covered by .idl variant
    for (const nodes::VariantField& field : v.fields) {
        if (field.protoField) {
            const auto* fd = descriptor->FindFieldByName(field.protoField->c_str());
            if (fd != nullptr) {
                protoVariantFieldNames.erase(fd->name());

                if (fd->is_required()) {
                    addError(field.name,
                        "Field is 'required' in Protobuf message");
                }

                checkFieldCompatibility(field.name, field.typeRef, fd);
            }
        }
    }

    // And finally, check if some fields were left "uncovered"
    if (!protoVariantFieldNames.empty()) {
        addError("", "Idl variant doesn't have fields [" +
            boost::algorithm::join(protoVariantFieldNames, ", ") +
            "] from Protobuf variant");
    }
}

void ErrorChecker::checkFieldCompatibility(
    const std::string& fieldName,
    const nodes::TypeRef& typeRef,
    const gp::FieldDescriptor* fieldDesc,
    bool insideVector)
{
    if (typeRef.id == nodes::TypeId::AnyCollection) {
        return; // Anything is convertible - no errors are possible!
    }

    if (typeRef.id == nodes::TypeId::Vector) {
        if (insideVector) {
            addError(fieldName,
                "'vector<vector<...>>' is not supported for Protobuf-based "
                "fields");
        } else {
            if (fieldDesc->is_repeated()) {
                checkFieldCompatibility(fieldName, typeRef.parameters[0],
                    fieldDesc, true);
            } else {
                addError(fieldName, "Protobuf type is not 'repeated'");
            }
        }
        return;
    }

    // If type is not vector (and not vector's type), but repeated
    if (!insideVector && fieldDesc->is_repeated()) {
        addError(fieldName, "Protobuf type is 'repeated'");
        return;
    }

    gp::FieldDescriptor::Type fieldDescType = fieldDesc->type();
    if (typeRef.id == nodes::TypeId::Custom) {
        const auto& type = idl_->type(scope_, *typeRef.name).type;
        if (fieldDescType == gp::FieldDescriptor::TYPE_ENUM) {
            checkEnumFieldCompatibility(
                fieldName, type, fieldDesc->enum_type());
        } else if (fieldDescType == gp::FieldDescriptor::TYPE_MESSAGE) {
            checkMessageFieldCompatibility(
                fieldName, type, fieldDesc->message_type());
        } else {
            addError(fieldName, "Protobuf type is not supported");
        }
        return;
    }

    if (typeRef.id == nodes::TypeId::Bool ||
            typeRef.id == nodes::TypeId::Int ||
            typeRef.id == nodes::TypeId::Uint ||
            typeRef.id == nodes::TypeId::Int64 ||
            typeRef.id == nodes::TypeId::Float ||
            typeRef.id == nodes::TypeId::Double ||
            typeRef.id == nodes::TypeId::String) {
        auto iterator = DESCRIPTOR_TO_POD.find(fieldDescType);
        if (iterator == DESCRIPTOR_TO_POD.end() ||
                iterator->second != typeRef.id) {
            addIncompatibleTypesError(fieldName);
        }
        return;
    } else if (typeRef.id == nodes::TypeId::Color) {
        if (fieldDescType != gp::FieldDescriptor::TYPE_UINT32 &&
                fieldDescType != gp::FieldDescriptor::TYPE_FIXED32) {
            addIncompatibleTypesError(fieldName);
        }
        return;
    }

    // Type is anything else: void, object, time-related types, image,
    // unique_ptr / shared_ptr or dictionary
    addError(fieldName, "Type not allowed for field based on Protobuf field");
}

void ErrorChecker::checkEnumFieldCompatibility(
    const std::string& fieldName,
    const Type& type,
    const gp::EnumDescriptor* protoEnumDesc)
{
    const auto idlEnum = std::get_if<const nodes::Enum*>(&type);
    if (idlEnum) {
        checkFieldProtoReference(fieldName, **idlEnum, protoEnumDesc);
    } else {
        addError(fieldName, "Protobuf type is enum, but Idl's is not");
    }
}

void ErrorChecker::checkMessageFieldCompatibility(
    const std::string& fieldName,
    const Type& type,
    const gp::Descriptor* protoTypeDesc)
{
    const auto idlStruct = std::get_if<const nodes::Struct*>(&type);
    if (idlStruct) {
        checkFieldProtoReference(fieldName, **idlStruct, protoTypeDesc);
    } else {
        const auto idlVariant = std::get_if<const nodes::Variant*>(&type);
        if (idlVariant) {
            checkFieldProtoReference(fieldName, **idlVariant, protoTypeDesc);
        } else {
            addError(fieldName, "Protobuf type is message, but Idl's is "
                "neither struct nor variant");
        }
    }
}

template <typename Node>
void ErrorChecker::checkFieldProtoReference(
    const std::string& fieldName,
    const Node& fieldTypeNode,
    const typename ProtoDescriptor<Node>::type* protoTypeDesc)
{
    if (!fieldTypeNode.protoMessage) {
        addError(fieldName,
            "Idl field's type is not based on any Protobuf type");
        return;
    }

    ProtoFile fileOfProtoType(protoTypeDesc->file());
    try {
        const auto& config = idl_->env->config;
        ProtoFile fileOfIdlType(config.inProtoRoot, config.baseProtoPackage,
            fieldTypeNode.protoMessage->pathToProto);
        if (fileOfIdlType != fileOfProtoType) {
            addIncompatibleTypesError(fieldName);
            return;
        }

        const auto* foundDesc =
            fileOfProtoType.findProtoTypeDesc(fieldTypeNode);
        if (foundDesc == nullptr) {
            addError(fieldName, "Idl field type's Protobuf type not found");
        } else {
            if (foundDesc->full_name() != protoTypeDesc->full_name()) {
                addIncompatibleTypesError(fieldName);
            }
        }
    } catch(const utils::Exception& e) {
        addError(fieldName, std::string(e.what()));
    }
}

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
