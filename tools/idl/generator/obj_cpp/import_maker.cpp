#include "obj_cpp/import_maker.h"

#include "common/common.h"
#include "common/interface_maker.h"
#include "cpp/import_maker.h"
#include "obj_cpp/common.h"
#include "objc/common.h"
#include "objc/import_maker.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/root.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

ImportMaker::ImportMaker(
    const Idl* idl,
    const common::TypeNameMaker* typeNameMaker,
    bool isHeader)
    : common::ImportMaker(
          idl,
          typeNameMaker,
          {
              "ios",
              idl->objcFramework,
              idl->env->runtimeFramework()->objcFramework,
              "yandex"
          },
          4),
      isHeader_(isHeader)
{
}

void ImportMaker::addAll(
    const FullTypeRef& typeRef,
    bool withSerialization,
    bool isHeader)
{
    if (isHeader_) {
        if (typeRef.id() == nodes::TypeId::AnyCollection) {
            addObjCRuntimeImportPath("Collection.h");
        }
    } else {
        if (typeRef.id() == nodes::TypeId::Point) {
            addCppRuntimeImportPath("bindings/ios/point_to_native.h");
            addCppRuntimeImportPath("bindings/ios/point_to_platform.h");
        } else if (typeRef.id() == nodes::TypeId::ImageProvider) {
            addCppRuntimeImportPath("image/ios/image_provider_binding.h");
        } else if (typeRef.id() == nodes::TypeId::ViewProvider) {
            addCppRuntimeImportPath("ui_view/ios/view_provider_binding.h");
        } else if (typeRef.id() == nodes::TypeId::AnimatedImageProvider) {
            addCppRuntimeImportPath("image/ios/animated_image_provider_binding.h");
        } else if (typeRef.id() == nodes::TypeId::ModelProvider) {
            addCppRuntimeImportPath("model/ios/model_provider_binding.h");
        } else if (typeRef.id() == nodes::TypeId::AnimatedModelProvider) {
            addCppRuntimeImportPath("model/ios/animated_model_provider_binding.h");
        } else if (typeRef.id() == nodes::TypeId::Vector) {
            addCppRuntimeImportPath("bindings/ios/vector_to_native.h");
            addCppRuntimeImportPath("bindings/ios/vector_to_platform.h");
        } else if (typeRef.id() == nodes::TypeId::Dictionary) {
            addCppRuntimeImportPath("bindings/ios/dictionary_to_native.h");
            addCppRuntimeImportPath("bindings/ios/dictionary_to_platform.h");
        } else if (typeRef.id() == nodes::TypeId::AnyCollection) {
            addCppRuntimeImportPath("any/ios/to_native.h");
            addCppRuntimeImportPath("any/ios/to_platform.h");
        } else if (typeRef.id() == nodes::TypeId::PlatformView) {
            addCppRuntimeImportPath("view/ios/to_native.h");
            addObjCRuntimeImportPath("PlatformView_Private.h");
        } else if (typeRef.id() == nodes::TypeId::Custom) {
            const auto& typeInfo = typeRef.info();

            addImportPath(objc::filePath(typeInfo.idl, true).inAngles());
            if (!std::holds_alternative<const nodes::Enum*>(typeInfo.type)) {
                addImportPath(filePath(typeInfo.idl, true).inAngles());

                if (std::holds_alternative<const nodes::Interface*>(typeInfo.type)) {
                    addImportPath(filePath(common::topMostBaseTypeInfo(&typeInfo)->idl, true).inAngles());
                }
            }
        }

        if (typeRef.isError()) {
            addCppRuntimeImportPath("ios/make_error.h");
        }
    }

    for (const auto& subRef : typeRef.subRefs()) {
        addAll(subRef, withSerialization, isHeader);
    }
}

void ImportMaker::fill(
    const std::string& selfImportPath,
    ctemplate::TemplateDictionary* parentDict)
{
    if (!isHeader_) {
        addCppRuntimeImportPath("bindings/ios/to_native.h");
        addCppRuntimeImportPath("bindings/ios/to_platform.h");
        addCppRuntimeImportPath("ios/exception.h");
    }

    common::ImportMaker::fill(selfImportPath, parentDict);
}

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
