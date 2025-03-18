#include "cpp/import_maker.h"

#include "common/common.h"
#include "cpp/import_maker.h"
#include "cpp/common.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

ImportMaker::ImportMaker(
    const Idl* idl,
    const common::TypeNameMaker* typeNameMaker)
    : common::ImportMaker(idl, typeNameMaker, { "yandex", "boost", "" }, 1)
{
}

namespace {

/**
 * Includes type's header, taking into account that it can be 'customized'.
 */
template <typename Node>
void includeTypeHeader(
    common::ImportMaker* importMaker,
    const TypeInfo& typeInfo)
{
    auto nodePtr = std::get_if<const Node*>(&typeInfo.type);
    if (nodePtr) {
        const Node& node = **nodePtr;
        if (node.customCodeLink.baseHeader) {
            importMaker->addImportPath(
                '<' + *node.customCodeLink.baseHeader + '>');
        } else {
            importMaker->addImportPath(filePath(typeInfo, true).inAngles());
        }
    }
}

} // namespace

void ImportMaker::addAll(
    const FullTypeRef& typeRef,
    bool withSerialization,
    bool dontImportIfPossible)
{
    if (withSerialization) {
        addImportPath("<boost/serialization/nvp.hpp>");
        addCppRuntimeImportPath("serialization/ptr.h");
    }

    if (typeRef.isOptional()) {
        if (withSerialization) {
            if (typeRef.env()->config.useStdOptional) {
                addImportPath("<yandex/maps/runtime/serialization/serialization_std.h>");
            } else {
                addImportPath("<boost/serialization/optional.hpp>");
            }
        }
        if (typeRef.env()->config.useStdOptional) {
            addImportPath("<optional>");
        } else {
            addImportPath("<boost/optional.hpp>");
        }
    }

    if (typeRef.id() == nodes::TypeId::Int64) {
        addImportPath("<cstdint>");
    } else if (typeRef.id() == nodes::TypeId::TimeInterval ||
            typeRef.id() == nodes::TypeId::AbsTimestamp ||
            typeRef.id() == nodes::TypeId::RelTimestamp) {
        addCppRuntimeImportPath("time.h");
        if (withSerialization) {
            addCppRuntimeImportPath("serialization/chrono.h");
        }
    } else if (typeRef.id() == nodes::TypeId::String) {
        addImportPath("<string>");
        if (withSerialization) {
            addImportPath("<boost/serialization/string.hpp>");
        }
    } else if (typeRef.id() == nodes::TypeId::Point) {
        addCppRuntimeImportPath("bindings/point_traits.h");

        addImportPath("<Eigen/Geometry>");
        if (withSerialization) {
            addCppRuntimeImportPath("serialization/math.h");
        }
    } else if (typeRef.id() == nodes::TypeId::Bitmap) {
        addCppRuntimeImportPath("platform_bitmap.h");
    } else if (typeRef.id() == nodes::TypeId::ImageProvider) {
        addCppRuntimeImportPath("image/image_provider.h");
    } else if (typeRef.id() == nodes::TypeId::ViewProvider) {
        addCppRuntimeImportPath("ui_view/view_provider.h");
    } else if (typeRef.id() == nodes::TypeId::AnimatedImageProvider) {
        addCppRuntimeImportPath("image/animated_image_provider.h");
    } else if (typeRef.id() == nodes::TypeId::ModelProvider) {
        addCppRuntimeImportPath("model/model_provider.h");
    } else if (typeRef.id() == nodes::TypeId::AnimatedModelProvider) {
        addCppRuntimeImportPath("model/animated_model_provider.h");
    } else if (typeRef.id() == nodes::TypeId::Bytes) {
        addImportPath("<cstdint>");
        addImportPath("<vector>");
        if (withSerialization) {
            addImportPath("<boost/serialization/vector.hpp>");
        }
    } else if (typeRef.id() == nodes::TypeId::Color) {
        addCppRuntimeImportPath("color.h");
    } else if (typeRef.id() == nodes::TypeId::Vector) {
        addCppRuntimeImportPath("bindings/platform.h");
        addImportPath("<memory>");
        if (withSerialization) {
            addImportPath("<boost/serialization/vector.hpp>");
        }
    } else if (typeRef.id() == nodes::TypeId::Dictionary) {
        addCppRuntimeImportPath("bindings/platform.h");
        addImportPath("<memory>");
        if (withSerialization) {
            addImportPath("<boost/serialization/map.hpp>");
        }
    } else if (typeRef.id() == nodes::TypeId::Any) {
        addImportPath("<boost/any.hpp>");
    } else if (typeRef.id() == nodes::TypeId::AnyCollection) {
        addCppRuntimeImportPath("any/collection.h");
    } else if (typeRef.id() == nodes::TypeId::PlatformView) {
        addCppRuntimeImportPath("view/platform_view.h");
    } else if (typeRef.id() == nodes::TypeId::Custom) {
        const auto& typeInfo = typeRef.info();
        auto headerPath = filePath(typeInfo, true).inAngles();

        // Enums
        includeTypeHeader<nodes::Enum>(this, typeInfo);

        // Interfaces
        auto i = std::get_if<const nodes::Interface*>(&typeInfo.type);
        if (i) {
            if ((**i).ownership != nodes::Interface::Ownership::Weak) {
                addImportPath("<memory>"); // std::[ unique / shared ]_ptr
            }

            bool willForwardDeclare = dontImportIfPossible &&
                addForwardDeclaration(FullTypeRef(typeInfo), headerPath);
            if (!willForwardDeclare) {
                addImportPath(headerPath);
            }
            return;
        }

        // Listeners
        auto l = std::get_if<const nodes::Listener*>(&typeInfo.type);
        if (l) {
            if (!(**l).isLambda) {
                addImportPath("<memory>"); // std::shared_ptr
            }

            addImportPath(headerPath);
            return;
        }

        // Structs
        includeTypeHeader<nodes::Struct>(this, typeInfo);

        // Variants
        includeTypeHeader<nodes::Variant>(this, typeInfo);
    }

    for (const auto& subRef : typeRef.subRefs()) {
        addAll(subRef, withSerialization);
    }
}

bool ImportMaker::addForwardDeclaration(
    const FullTypeRef& typeRef,
    const std::string& importPath)
{
    if (typeRef.info().scope.original().isEmpty()) {
        return common::ImportMaker::addForwardDeclaration(
            typeRef, importPath);
    } else {
        return false;
    }
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
