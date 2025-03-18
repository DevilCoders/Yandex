#include "cpp/type_name_maker.h"

#include "common/common.h"
#include "cpp/common.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/root.h>

#include <algorithm>
#include <unordered_map>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

std::string fullName(const Scope& fullNameAsScope)
{
    return "::" + fullNameAsScope.asString("::");
}
std::string fullName(const FullTypeRef& typeRef)
{
    return fullName(typeRef.info().fullNameAsScope[CPP]);
}

std::string nativeName(const TypeInfo& typeInfo)
{
    return typeInfo.fullNameAsScope[CPP].asString("::");
}

std::string listenerBindingTypeName(
    const std::string& platform,
    const TypeInfo& typeInfo)
{
    return fullName(
        typeInfo.idl->cppNamespace + platform + (typeInfo.name[CPP] + "Binding"));
}

std::string TypeNameMaker::make(const FullTypeRef& typeRef, bool makeRef) const
{
    static const std::unordered_map<nodes::TypeId, std::string> LABELS {
        { nodes::TypeId::Void,   "void"                      },
        { nodes::TypeId::Bool,   "bool"                      },
        { nodes::TypeId::Int,    "int"                       },
        { nodes::TypeId::Uint,   "unsigned int"              },
        { nodes::TypeId::Int64,  "std::int64_t"              },
        { nodes::TypeId::Float,  "float"                     },
        { nodes::TypeId::Double, "double"                    },
        { nodes::TypeId::String, "std::string"               },
        { nodes::TypeId::Bytes,  "std::vector<std::uint8_t>" },
        { nodes::TypeId::Point,  "Eigen::Vector2f"           },
        { nodes::TypeId::Any,    "boost::any"                }
    };

    std::string smartPtr, coreName;
    const auto& runtimeNamespace = typeRef.env()->runtimeFramework()->cppNamespace;
    if (typeRef.id() == nodes::TypeId::TimeInterval) {
        coreName = fullName(runtimeNamespace + "TimeInterval");
    } else if (typeRef.id() == nodes::TypeId::AbsTimestamp) {
        coreName = fullName(runtimeNamespace + "AbsoluteTimestamp");
    } else if (typeRef.id() == nodes::TypeId::RelTimestamp) {
        coreName = fullName(runtimeNamespace + "RelativeTimestamp");
    } else if (typeRef.id() == nodes::TypeId::Color) {
        coreName = fullName(runtimeNamespace + "Color");
    } else if (typeRef.id() == nodes::TypeId::Bitmap) {
        coreName = fullName(runtimeNamespace + "PlatformBitmap");
    } else if (typeRef.id() == nodes::TypeId::ImageProvider) {
        smartPtr = "std::unique_ptr";
        coreName = fullName(runtimeNamespace + "image" + "ImageProvider");
    } else if (typeRef.id() == nodes::TypeId::AnimatedImageProvider) {
        smartPtr = "std::unique_ptr";
        coreName = fullName(runtimeNamespace + "image" + "AnimatedImageProvider");
    } else if (typeRef.id() == nodes::TypeId::ModelProvider) {
        smartPtr = "std::unique_ptr";
        coreName = fullName(runtimeNamespace + "model" + "ModelProvider");
    } else if (typeRef.id() == nodes::TypeId::AnimatedModelProvider) {
        smartPtr = "std::unique_ptr";
        coreName = fullName(runtimeNamespace + "model" + "AnimatedModelProvider");
    } else if (typeRef.id() == nodes::TypeId::ViewProvider) {
        smartPtr = "std::unique_ptr";
        coreName = fullName(runtimeNamespace + "ui_view" + "ViewProvider");
    } else if (typeRef.id() == nodes::TypeId::Vector) {
        auto paramName = make(typeRef.vectorItem(), makeRef);

        std::size_t sharedPtrOffset = 0;
        if (paramName.starts_with("std::shared_ptr<")) {
            sharedPtrOffset = 15;
        } else if (paramName.starts_with("const std::shared_ptr<")) {
            sharedPtrOffset = 21;
        }
        if (sharedPtrOffset > 0) {
            coreName = fullName(runtimeNamespace + "bindings" + "SharedVector");
            coreName += paramName.substr(sharedPtrOffset);
        } else {
            coreName = fullName(runtimeNamespace + "bindings" + "Vector");
            coreName += '<' + paramName + '>';
        }
    } else if (typeRef.id() == nodes::TypeId::Dictionary) {
        coreName = fullName(runtimeNamespace + "bindings" + "StringDictionary");
        coreName += '<' + make(typeRef.dictValue(), makeRef) + '>';
    } else if (typeRef.id() == nodes::TypeId::AnyCollection) {
        coreName = fullName(runtimeNamespace + "any" + "Collection");
    } else if (typeRef.id() == nodes::TypeId::PlatformView) {
        coreName = fullName(runtimeNamespace + "view" + "PlatformView") +
            (makeRef ? "*" : "");
    } else if (typeRef.id() == nodes::TypeId::Custom) {
        const auto& typeInfo = typeRef.info();

        coreName = fullName(typeInfo.fullNameAsScope[CPP]);

        auto i = std::get_if<const nodes::Interface*>(&typeInfo.type);
        if (i) {
            const auto ownership = (**i).ownership;
            if (ownership == nodes::Interface::Ownership::Strong) {
                smartPtr = "std::unique_ptr";
            } else if (ownership == nodes::Interface::Ownership::Shared) {
                smartPtr = "std::shared_ptr";
            } else {
                if (makeRef) {
                    coreName += "*";
                }
            }
        } else {
            auto l = std::get_if<const nodes::Listener*>(&typeInfo.type);
            if (l && !(**l).isLambda) {
                smartPtr = "const std::shared_ptr";
            }
        }
    } else {
        coreName = LABELS.at(typeRef.id());
    }

    if (typeRef.isBridged()) {
        if (makeRef) {
            coreName = "std::shared_ptr<" + coreName + ">";
        }
    } else if (typeRef.isOptional() && !(typeRef.isCppNullable())) {
        if (typeRef.env()->config.useStdOptional) {
            coreName = "std::optional<" + coreName + '>';
        } else {
            coreName = "boost::optional<" + coreName + '>';
        }
    }

    if (smartPtr.length() > 0 && makeRef) {
        coreName = smartPtr + '<' + coreName + '>';
    }
    if (typeRef.isConst() && makeRef) {
        coreName = "const " + coreName;
    }
    return coreName;
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
