#include "objc/import_maker.h"

#include "cpp/common.h"
#include "objc/common.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/root.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

ImportMaker::ImportMaker(
    const Idl* idl,
    const common::TypeNameMaker* typeNameMaker,
    bool isHeader)
    : common::ImportMaker(
          idl,
          typeNameMaker,
          {
              idl->objcFramework,
              idl->env->runtimeFramework()->objcFramework
          },
          2),
      isHeader_(isHeader)
{
}

void ImportMaker::addAll(
    const FullTypeRef& typeRef,
    bool withSerialization,
    bool isHeader)
{
    if (isHeader_) {
        if (typeRef.id() == nodes::TypeId::Color ||
                typeRef.id() == nodes::TypeId::Point ||
                typeRef.id() == nodes::TypeId::Bitmap ||
                typeRef.id() == nodes::TypeId::ImageProvider) {
            addImportPath("<UIKit/UIKit.h>");
        } else if (typeRef.id() == nodes::TypeId::ViewProvider) {
            addObjCRuntimeImportPath("ViewProvider.h");
        } else if (typeRef.id() == nodes::TypeId::AnimatedImageProvider) {
            addObjCRuntimeImportPath("AnimatedImageProvider.h");
        } else if (typeRef.id() == nodes::TypeId::ModelProvider) {
            addObjCRuntimeImportPath("ModelProvider.h");
        } else if (typeRef.id() == nodes::TypeId::AnimatedModelProvider) {
            addObjCRuntimeImportPath("AnimatedModelProvider.h");
        } else if (typeRef.id() == nodes::TypeId::AnyCollection) {
            addObjCRuntimeImportPath("Collection.h");
        } else if (typeRef.id() == nodes::TypeId::PlatformView) {
            if (isHeader) {
                addObjCRuntimeImportPath("PlatformView_Fwd.h");
            } else {
                addObjCRuntimeImportPath("PlatformView.h");
            }
        } else if (typeRef.id() == nodes::TypeId::Custom) {
            if (typeRef.isError()) {
                return;
            }

            const auto& typeInfo = typeRef.info();
            auto headerPath = filePath(typeInfo.idl, true).inAngles();

            bool willForwardDeclare = isHeader &&
                (
                    std::holds_alternative<const nodes::Interface*>(typeInfo.type) ||
                    std::holds_alternative<const nodes::Struct*>(typeInfo.type)
                ) &&
                addForwardDeclaration(typeRef, headerPath);
            if (!willForwardDeclare) {
                addImportPath(headerPath);
            }
        }

        for (const auto& subRef : typeRef.subRefs()) {
            addAll(subRef, withSerialization, isHeader);
        }
    } else if (withSerialization) {
        addObjCRuntimeImportPath("Serialization.h");
        addObjCRuntimeImportPath("ArchivingUtilities.h");
    }
}

bool ImportMaker::addForwardDeclaration(
    const FullTypeRef& typeRef,
    const std::string& importPath)
{
    if (typeRef.info().idl->idlNamespace == idl_->idlNamespace) {
        auto typeName = typeNameMaker_->makeConstructorName(typeRef.info());
        forwardDeclarations_.insert({ typeName, importPath, &typeRef.info() });
        return true;
    } else {
        return false;
    }
}

void ImportMaker::fill(
    const std::string& selfImportPath,
    ctemplate::TemplateDictionary* dict)
{
    importPaths_.erase(selfImportPath);
    if (importPaths_.empty()) {
        importPaths_.insert("<Foundation/Foundation.h>");
    }
    addObjCRuntimeImportPath("Export.h");

    common::ImportMaker::fill(selfImportPath, dict);
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
