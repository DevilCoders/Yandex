#include "java/type_name_maker.h"

#include "common/common.h"

#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/utils/exception.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

std::string TypeNameMaker::make(const FullTypeRef& typeRef) const
{
    switch (typeRef.id()) {
        case nodes::TypeId::Void:
            return "void";
        case nodes::TypeId::Bool:
            return typeRef.isOptional() ? "Boolean" : "boolean";
        case nodes::TypeId::Int:
        case nodes::TypeId::Uint:
        case nodes::TypeId::Color:
            return typeRef.isOptional() ? "Integer" : "int";
        case nodes::TypeId::Int64:
            return typeRef.isOptional() ? "Long" : "long";
        case nodes::TypeId::Float:
            return typeRef.isOptional() ? "Float" : "float";
        case nodes::TypeId::Double:
            return typeRef.isOptional() ? "Double" : "double";
        case nodes::TypeId::String:
            return "String";
        case nodes::TypeId::TimeInterval:
        case nodes::TypeId::AbsTimestamp:
        case nodes::TypeId::RelTimestamp:
            return typeRef.isOptional() ? "Long" : "long";
        case nodes::TypeId::Bytes:
            return "byte[]";
        case nodes::TypeId::Point:
            return "PointF";
        case nodes::TypeId::Bitmap:
            return "Bitmap";
        case nodes::TypeId::ImageProvider:
            return "ImageProvider";
        case nodes::TypeId::AnimatedImageProvider:
            return "AnimatedImageProvider";
        case nodes::TypeId::ModelProvider:
            return "ModelProvider";
        case nodes::TypeId::AnimatedModelProvider:
            return "AnimatedModelProvider";
        case nodes::TypeId::ViewProvider:
            return "ViewProvider";
        case nodes::TypeId::Vector:
            return "List<" + make(typeRef.vectorItem().asOptional()) + '>';
        case nodes::TypeId::Dictionary:
            return "Map<" +
                make(typeRef.dictKey().asOptional()) + ", " +
                make(typeRef.dictValue().asOptional()) + '>';
        case nodes::TypeId::Any:
            return "Object";
        case nodes::TypeId::AnyCollection:
            return "Collection";
        case nodes::TypeId::PlatformView:
            return "PlatformView";
        case nodes::TypeId::Custom:
            return typeRef.info().name[JAVA];
        default:
            INTERNAL_ERROR("Couldn't recognize Idl type id: " <<
                typeRef.id());
    }
}

std::string TypeNameMaker::makeRef(const FullTypeRef& typeRef) const
{
    if (typeRef.isBitfieldEnum()) {
        return typeRef.isOptional() ? "Integer" : "int";
    }

    return make(typeRef);
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
