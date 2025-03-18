#include "objc/type_name_maker.h"

#include "common/common.h"
#include "objc/common.h"

#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/utils/exception.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

namespace {

std::string asRef(std::string base, bool makeRef)
{
    return makeRef ? base + " *" : base;
}

} // namespace

std::string TypeNameMaker::makeConstructorName(const TypeInfo& typeInfo) const
{
    auto prefix = typeInfo.idl->objcTypePrefix.asString("");
    if (typeInfo.scope[OBJC].isEmpty()) {
        return prefix + typeInfo.name[OBJC];
    } else {
        return prefix + typeInfo.scope[OBJC].last() + typeInfo.name[OBJC];
    }
}
std::string TypeNameMaker::makeInstanceName(const TypeInfo& typeInfo) const
{
    return utils::unCapitalizeWord(typeInfo.name[OBJC]);
}

std::string TypeNameMaker::make(const FullTypeRef& typeRef, bool makeRef) const
{
    auto runtimePrefix = runtimeFrameworkPrefix(typeRef.env());

    switch (typeRef.id()) {
        case nodes::TypeId::Void:
            return "void";
        case nodes::TypeId::Bool:
            return typeRef.isOptional() ? asRef("NSNumber", makeRef) : "BOOL";
        case nodes::TypeId::Int:
            return typeRef.isOptional() ? asRef("NSNumber", makeRef) : "NSInteger";
        case nodes::TypeId::Uint:
            return typeRef.isOptional() ? asRef("NSNumber", makeRef) : "NSUInteger";
        case nodes::TypeId::Int64:
            return typeRef.isOptional() ? asRef("NSNumber", makeRef) : "long long";
        case nodes::TypeId::Float:
            return typeRef.isOptional() ? asRef("NSNumber", makeRef) : "float";
        case nodes::TypeId::Double:
            return typeRef.isOptional() ? asRef("NSNumber", makeRef) : "double";
        case nodes::TypeId::String:
            return asRef("NSString", makeRef);
        case nodes::TypeId::TimeInterval:
            return typeRef.isOptional() ? asRef("NSNumber", makeRef) : "NSTimeInterval";
        case nodes::TypeId::AbsTimestamp:
        case nodes::TypeId::RelTimestamp:
            return asRef("NSDate", makeRef);
        case nodes::TypeId::Bytes:
            return asRef("NSData", makeRef);
        case nodes::TypeId::Color:
            return asRef("UIColor", makeRef);
        case nodes::TypeId::Point:
            return typeRef.isOptional() ? asRef("NSValue", makeRef) : "CGPoint";
        case nodes::TypeId::Bitmap:
        case nodes::TypeId::ImageProvider:
            return asRef("UIImage", makeRef);
        case nodes::TypeId::AnimatedImageProvider:
            return "id<" + runtimePrefix + "AnimatedImageProvider>";
        case nodes::TypeId::ModelProvider:
            return "id<" + runtimePrefix + "ModelProvider>";
        case nodes::TypeId::AnimatedModelProvider:
            return "id<" + runtimePrefix + "AnimatedModelProvider>";
        case nodes::TypeId::ViewProvider:
            return asRef(runtimePrefix + "ViewProvider", makeRef);
        case nodes::TypeId::Vector:
        {
            return asRef("NSArray<" + make(typeRef.vectorItem().asOptional(), makeRef) + ">", makeRef);
        }
        case nodes::TypeId::Dictionary:
            return asRef("NSDictionary<" + make(typeRef.dictKey().asOptional(), makeRef) + ", " +
                make(typeRef.dictValue().asOptional(), makeRef) + ">", makeRef);
        case nodes::TypeId::Any:
            return "id";
        case nodes::TypeId::AnyCollection:
            return asRef(runtimePrefix + "Collection", makeRef);
        case nodes::TypeId::PlatformView:
            return "id<" + runtimePrefix + "PlatformView>";
        case nodes::TypeId::Custom:
        {
            if (typeRef.isError()) {
                return makeRef ? "NSError *" : runtimePrefix + "Error";
            }

            if (typeRef.isOptional() && typeRef.is<nodes::Enum>()) {
                return asRef("NSNumber", makeRef);
            }

            auto typeName = typeRef.info().fullNameAsScope[OBJC].asString("");
            if (typeRef.is<nodes::Enum>() || typeRef.isLambdaListener()) {
                return typeName;
            } else if (typeRef.isClassicListener()) {
                return makeRef ? ("id<" + typeName + '>') : typeName;
            } else {
                return asRef(typeName, makeRef);
            }
        }
        default:
            INTERNAL_ERROR("Couldn't recognize Idl type id: " << typeRef.id());
    }
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
