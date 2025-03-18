#pragma once

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/type_info.h>

#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

/**
 * Returns JNI type name:
 *     if isGlobalRef is false - "jobject" for custom types, "jfloat" for
 *         floats, "jstring" for strings...
 *     if true - "runtime::android::JniObject" for custom types, "jfloat" for
 *         floats, "runtime::android::JniString" for strings...
 *
 * Note the difference between our concepts of JNI type name and JNI type name
 * reference!
 */
std::string jniTypeName(const FullTypeRef& typeRef, bool isGlobalRef);

/**
 * Returns possibly shortened JNI type name reference, e.g. "java/lang/String"
 * for TypeId::String or "I" for TypeId::Int.
 *
 * Normally, references to custom types have 'L' at front and ';' at the end,
 * and shortened refs won't have them. These shortened refs appear in our
 * bindings quite often.
 */
std::string jniTypeRef(const FullTypeRef& typeRef, bool isShortened);

/**
 * Returns possibly shortened JNI type name reference for a custom type with
 * given info. About reference shortening see the above method.
 */
std::string jniTypeRef(
    const TypeInfo& typeInfo,
    bool isShortened,
    bool isInterfaceBinding);

/**
 * In JNI method names, type references are mangled - to become part of a
 * valid C-language identifier.
 */
std::string mangleJniTypeRefs(std::string jniTypeRefsString);

/**
 * Returns JNI method name's part where parameters are. It is a long string
 * with all JNI parameter type references in a mangled form.
 */
std::string jniMethodNameParametersPart(
    const FullScope& scope,
    const std::vector<nodes::FunctionParameter>& parameters);

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
