#include "jni_cpp/jni.h"

#include "common/common.h"
#include "cpp/type_name_maker.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/targets.h>

#include <boost/algorithm/string.hpp>

#include <utility>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

namespace {

std::string fullRuntimeTypeName(
    const Environment* env,
    std::string relativeTypeName)
{
    const auto& runtimeNamespace = env->runtimeFramework()->cppNamespace;
    return cpp::fullName(runtimeNamespace + relativeTypeName);
}

} // namespace

std::string jniTypeName(const FullTypeRef& typeRef, bool isGlobalRef)
{
    if (typeRef.isOptional() && typeRef.id() != nodes::TypeId::String) {
        return isGlobalRef ?
            fullRuntimeTypeName(typeRef.env(), "android::JniObject") : "jobject";
    } else if (typeRef.id() == nodes::TypeId::Void) {
        return "void";
    } else if (typeRef.id() == nodes::TypeId::Bool) {
        return "jboolean";
    } else if (typeRef.id() == nodes::TypeId::Int ||
            typeRef.id() == nodes::TypeId::Uint ||
            typeRef.id() == nodes::TypeId::Color) {
        return "jint";
    } else if (typeRef.id() == nodes::TypeId::Int64 ||
            typeRef.id() == nodes::TypeId::TimeInterval ||
            typeRef.id() == nodes::TypeId::AbsTimestamp ||
            typeRef.id() == nodes::TypeId::RelTimestamp) {
        return "jlong";
    } else if (typeRef.id() == nodes::TypeId::Float) {
        return "jfloat";
    } else if (typeRef.id() == nodes::TypeId::Double) {
        return "jdouble";
    } else if (typeRef.id() == nodes::TypeId::String) {
        return isGlobalRef ?
            fullRuntimeTypeName(typeRef.env(), "android::JniString") : "jstring";
    } else if (typeRef.id() == nodes::TypeId::Custom) {
        if (!typeRef.isOptional() && typeRef.isBitfieldEnum()) {
            return "jint";
        }
        return isGlobalRef ?
            fullRuntimeTypeName(typeRef.env(), "android::JniObject") : "jobject";
    } else {
        return isGlobalRef ?
            fullRuntimeTypeName(typeRef.env(), "android::JniObject") : "jobject";
    }
}

namespace {

/**
 * Conditionally converts base jni type name to its long form - appends 'L'
 * and ';' - but only if keepShortened is false.
 */
std::string toLongRef(std::string baseName, bool keepShortened)
{
    return keepShortened ? baseName : 'L' + baseName + ';';
}

} // namespace

std::string jniTypeRef(const FullTypeRef& typeRef, bool isShortened)
{
    auto runtimeJavaPrefix =
        typeRef.env()->runtimeFramework()->javaPackage.asPrefix("/");

    if (typeRef.id() == nodes::TypeId::Void) {
        return "V";
    } else if (typeRef.id() == nodes::TypeId::Bool) {
        return typeRef.isOptional() ?
            toLongRef("java/lang/Boolean", isShortened) : "Z";
    } else if (typeRef.id() == nodes::TypeId::Int ||
            typeRef.id() == nodes::TypeId::Uint ||
            typeRef.id() == nodes::TypeId::Color) {
        return typeRef.isOptional() ?
            toLongRef("java/lang/Integer", isShortened) : "I";
    } else if (typeRef.id() == nodes::TypeId::Int64 ||
            typeRef.id() == nodes::TypeId::TimeInterval ||
            typeRef.id() == nodes::TypeId::AbsTimestamp ||
            typeRef.id() == nodes::TypeId::RelTimestamp) {
        return typeRef.isOptional() ? toLongRef("java/lang/Long", isShortened) : "J";
    } else if (typeRef.id() == nodes::TypeId::Float) {
        return typeRef.isOptional() ? toLongRef("java/lang/Float", isShortened) : "F";
    } else if (typeRef.id() == nodes::TypeId::Double) {
        return typeRef.isOptional() ? toLongRef("java/lang/Double", isShortened) : "D";
    } else if (typeRef.id() == nodes::TypeId::String) {
        return toLongRef("java/lang/String", isShortened);
    } else if (typeRef.id() == nodes::TypeId::Point) {
        return toLongRef("android/graphics/PointF", isShortened);
    } else if (typeRef.id() == nodes::TypeId::Bitmap) {
        return toLongRef("android/graphics/Bitmap", isShortened);
    } else if (typeRef.id() == nodes::TypeId::ImageProvider) {
        return toLongRef(
            runtimeJavaPrefix + "image/ImageProvider", isShortened);
    } else if (typeRef.id() == nodes::TypeId::ViewProvider) {
        return toLongRef(
            runtimeJavaPrefix + "ui_view/ViewProvider", isShortened);
    } else if (typeRef.id() == nodes::TypeId::AnimatedImageProvider) {
        return toLongRef(
            runtimeJavaPrefix + "image/AnimatedImageProvider", isShortened);
    } else if (typeRef.id() == nodes::TypeId::ModelProvider) {
        return toLongRef(
            runtimeJavaPrefix + "model/ModelProvider", isShortened);
    } else if (typeRef.id() == nodes::TypeId::AnimatedModelProvider) {
        return toLongRef(
            runtimeJavaPrefix + "model/AnimatedModelProvider", isShortened);
    } else if (typeRef.id() == nodes::TypeId::Bytes) {
        return "[B";
    } else if (typeRef.id() == nodes::TypeId::Vector) {
        return toLongRef("java/util/List", isShortened);
    } else if (typeRef.id() == nodes::TypeId::Dictionary) {
        return toLongRef("java/util/Map", isShortened);
    } else if (typeRef.id() == nodes::TypeId::Any) {
        return toLongRef("java/lang/Object", isShortened);
    } else if (typeRef.id() == nodes::TypeId::AnyCollection) {
        return toLongRef(runtimeJavaPrefix + "any/Collection", isShortened);
    } else if (typeRef.id() == nodes::TypeId::PlatformView) {
        return toLongRef(runtimeJavaPrefix + "view/PlatformView", isShortened);
    } else { // TypeId::Custom
        auto e = typeRef.as<nodes::Enum>();
        if (e && e->isBitField) {
            return typeRef.isOptional() ?
                toLongRef("java/lang/Integer", isShortened) : "I";
        } else {
            return jniTypeRef(typeRef.info(), isShortened, false);
        }
    }
}

std::string jniTypeRef(
    const TypeInfo& typeInfo,
    bool isShortened,
    bool isInterfaceBinding)
{
    std::string interfaceNameSuffix = isInterfaceBinding ? "Binding" : "";
    std::string delimiter = interfaceNameSuffix + "$";

    auto baseName =
        typeInfo.idl->javaPackage.asPrefix("/") +
        (isInterfaceBinding ? "internal/" : "") +
        typeInfo.scope[JAVA].asPrefix(delimiter) +
        typeInfo.name[JAVA] + interfaceNameSuffix;
    return toLongRef(baseName, isShortened);
}

std::string mangleJniTypeRefs(std::string jniTypeRefsString)
{
    // First and last transformations must always remain first and last!
    static const std::pair<std::string, std::string> TRANSFORMATIONS[] = {
        { "_", "_1" },
        { ";", "_2" },
        { "[", "_3" },
        { "$", "_00024" },
        { "/", "_" }
    };

    for (const auto& t : TRANSFORMATIONS) {
        boost::replace_all(jniTypeRefsString, t.first, t.second);
    }
    return jniTypeRefsString;
}

std::string jniMethodNameParametersPart(
    const FullScope& scope,
    const std::vector<nodes::FunctionParameter>& parameters)
{
    if (parameters.empty()) {
        return "__";
    }

    std::string jniParameterTypeRefsString;
    for (const auto& p : parameters) {
        jniParameterTypeRefsString += jniTypeRef({ scope, p.typeRef }, false);
    }
    return "__" + mangleJniTypeRefs(jniParameterTypeRefsString);
}

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
