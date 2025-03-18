#include "java/import_maker.h"

#include "common/common.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

ImportMaker::ImportMaker(
    const Idl* idl,
    const common::TypeNameMaker* typeNameMaker,
    bool isBinding)
    : common::ImportMaker(
          idl, typeNameMaker, { "ru", "android", "javax", "java" }, 1),
      isBinding_(isBinding)
{
}

namespace {

class SerializationImporter {
public:
    SerializationImporter(common::ImportMaker* importMaker)
        : importMaker_(importMaker)
    {
    }
    void operator()(const nodes::Struct*) const
    {
        importMaker_->addJavaRuntimeImportPath("bindings.ClassHandler");
    }
    void operator()(const nodes::Variant*) const
    {
        importMaker_->addJavaRuntimeImportPath("bindings.ClassHandler");
    }
    void operator()(const nodes::Interface*) const {}
    void operator()(const nodes::Listener*) const {}
    void operator()(const nodes::Enum*) const
    {
        importMaker_->addJavaRuntimeImportPath("bindings.EnumHandler");
    }

private:
    common::ImportMaker* importMaker_;
};

/**
 * Checks whether we need to generate java import for given type reference.
 * We don't need to import only if type's package equals current file's
 * package, and type's scope equals or is "above" file's current scope.
 */
bool isImportNeeded(
    const Scope& fileJavaPackage,
    const Scope& typeJavaPackage,
    const Scope& fileScope,
    const Scope& typeScope)
{
    if (typeJavaPackage != fileJavaPackage) {
        return true;
    }

    if (fileScope.size() < typeScope.size()) {
        return true;
    }

    for (size_t i = 0; i < typeScope.size(); ++i) {
        if (typeScope[i] != fileScope[i]) {
            return true;
        }
    }

    return false;
}

} // namespace

void ImportMaker::addAll(
    const FullTypeRef& typeRef,
    bool withSerialization,
    bool /* isHeader */)
{
    if (withSerialization) {
        addJavaRuntimeImportPath("bindings.Archive");
        addJavaRuntimeImportPath("bindings.Serializable");
    }

    if (typeRef.id() == nodes::TypeId::Point) {
        addImportPath("android.graphics.PointF");
    } else if (typeRef.id() == nodes::TypeId::Bitmap) {
        addImportPath("android.graphics.Bitmap");
    } else if (typeRef.id() == nodes::TypeId::ImageProvider) {
        addJavaRuntimeImportPath("image.ImageProvider");
    } else if (typeRef.id() == nodes::TypeId::AnimatedImageProvider) {
        addJavaRuntimeImportPath("image.AnimatedImageProvider");
    } else if (typeRef.id() == nodes::TypeId::ModelProvider) {
        addJavaRuntimeImportPath("model.ModelProvider");
    } else if (typeRef.id() == nodes::TypeId::AnimatedModelProvider) {
        addJavaRuntimeImportPath("model.AnimatedModelProvider");
    } else if (typeRef.id() == nodes::TypeId::Vector) {
        addImportPath("java.util.List");
    } else if (typeRef.id() == nodes::TypeId::Dictionary) {
        addImportPath("java.util.Map");
    } else if (typeRef.id() == nodes::TypeId::AnyCollection) {
        addJavaRuntimeImportPath("any.Collection");
    } else if (typeRef.id() == nodes::TypeId::ViewProvider) {
        addJavaRuntimeImportPath("ui_view.ViewProvider");
    } else if (typeRef.id() == nodes::TypeId::PlatformView) {
        addJavaRuntimeImportPath("view.PlatformView");
    } else if (typeRef.id() == nodes::TypeId::Custom) {
        const auto& typeInfo = typeRef.info();
        if (isBinding_ || isImportNeeded(idl_->javaPackage, typeInfo.idl->javaPackage,
                typeRef.fullScope().scope, typeInfo.scope.original())) {
            addImportPath(typeInfo.fullNameAsScope[JAVA].asString("."));
        }
    }

    for (const auto& subRef : typeRef.subRefs()) {
        if (withSerialization) {
            if (subRef.id() == nodes::TypeId::Bool) {
                addJavaRuntimeImportPath("bindings.BooleanHandler");
            } else if (subRef.id() == nodes::TypeId::Int ||
                    subRef.id() == nodes::TypeId::Uint ||
                    subRef.id() == nodes::TypeId::Color) {
                addJavaRuntimeImportPath("bindings.IntegerHandler");
            } else if (subRef.id() == nodes::TypeId::Int64) {
                addJavaRuntimeImportPath("bindings.LongHandler");
            } else if (subRef.id() == nodes::TypeId::Float) {
                addJavaRuntimeImportPath("bindings.FloatHandler");
            } else if (subRef.id() == nodes::TypeId::Double) {
                addJavaRuntimeImportPath("bindings.DoubleHandler");
            } else if (subRef.id() == nodes::TypeId::String) {
                addJavaRuntimeImportPath("bindings.StringHandler");
            } else if (subRef.id() == nodes::TypeId::Bytes) {
                addJavaRuntimeImportPath("bindings.BytesHandler");
            } else if (subRef.id() == nodes::TypeId::TimeInterval ||
                    subRef.id() == nodes::TypeId::AbsTimestamp ||
                    subRef.id() == nodes::TypeId::RelTimestamp) {
                addJavaRuntimeImportPath("bindings.LongHandler");
            } else if (subRef.id() == nodes::TypeId::Point) {
                addJavaRuntimeImportPath("bindings.PointHandler");
            } else if (subRef.id() == nodes::TypeId::Custom) {
                std::visit(SerializationImporter(this), subRef.info().type);
            }
        }

        addAll(subRef, withSerialization);
    }
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
