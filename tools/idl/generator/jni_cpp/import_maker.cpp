#include "jni_cpp/import_maker.h"

#include "common/common.h"
#include "cpp/import_maker.h"
#include "jni_cpp/common.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/utils/paths.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

namespace {

bool needToIncludeBindingHeader(const TypeInfo& typeInfo)
{
    if (std::holds_alternative<const nodes::Listener*>(typeInfo.type)) {
        return true;
    }
    if (std::holds_alternative<const nodes::Struct*>(typeInfo.type)) {
        return true;
    }
    if (std::holds_alternative<const nodes::Variant*>(typeInfo.type)) {
        return true;
    }

    return false;
}

} // namespace

void ImportMaker::addAll(
    const FullTypeRef& typeRef,
    bool /* withSerialization */,
    bool /* isHeader */)
{
    cpp::ImportMaker::addAll(typeRef, false);

    if (typeRef.id() == nodes::TypeId::Point) {
        addCppRuntimeImportPath("bindings/android/point_to_native.h");
        addCppRuntimeImportPath("bindings/android/point_to_platform.h");
    } else if (typeRef.id() == nodes::TypeId::ImageProvider) {
        addCppRuntimeImportPath("image/android/image_provider_binding.h");
    } else if (typeRef.id() == nodes::TypeId::AnimatedImageProvider) {
        addCppRuntimeImportPath("image/android/animated_image_provider_binding.h");
    } else if (typeRef.id() == nodes::TypeId::ModelProvider) {
        addCppRuntimeImportPath("model/android/model_provider_binding.h");
    } else if (typeRef.id() == nodes::TypeId::AnimatedModelProvider) {
        addCppRuntimeImportPath("model/android/animated_model_provider_binding.h");
    } else if (typeRef.id() == nodes::TypeId::ViewProvider) {
        addCppRuntimeImportPath("ui_view/android/view_provider_binding.h");
    } else if (typeRef.id() == nodes::TypeId::Vector) {
        addCppRuntimeImportPath("bindings/android/vector_to_native.h");
        addCppRuntimeImportPath("bindings/android/vector_to_platform.h");
    } else if (typeRef.id() == nodes::TypeId::Dictionary) {
        addCppRuntimeImportPath("bindings/android/dictionary_to_native.h");
        addCppRuntimeImportPath("bindings/android/dictionary_to_platform.h");
    } else if (typeRef.id() == nodes::TypeId::AnyCollection) {
        addCppRuntimeImportPath("any/android/to_native.h");
        addCppRuntimeImportPath("any/android/to_platform.h");
    } else if (typeRef.id() == nodes::TypeId::PlatformView) {
        addCppRuntimeImportPath("view/android/to_native.h");
    } else if (typeRef.id() == nodes::TypeId::Custom) {
        const auto& typeInfo = typeRef.info();
        if (needToIncludeBindingHeader(typeInfo)) {
            addImportPath(filePath(typeInfo, true).inAngles());
        }

        if (typeRef.isError()) {
            addCppRuntimeImportPath("android/make_error.h");
        }
    }
}

void ImportMaker::fill(
    const std::string& selfImportPath,
    ctemplate::TemplateDictionary* parentDict)
{
    // Include for "own" header file is added separately (used only in .cpp)
    importPaths_.erase(utils::Path(selfImportPath).stem().withExtension("h"));

    addCppRuntimeImportPath("android/object.h");
    addCppRuntimeImportPath("bindings/android/to_native.h");
    addCppRuntimeImportPath("bindings/android/to_platform.h");
    addCppRuntimeImportPath("exception.h");

    cpp::ImportMaker::fill(selfImportPath, parentDict);
}

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
