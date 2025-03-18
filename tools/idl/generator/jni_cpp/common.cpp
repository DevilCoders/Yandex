#include "jni_cpp/common.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

utils::Path filePath(const Idl* idl, bool isHeader)
{
    return idl->cppNamespace.asPath() + "internal/android" +
        idl->relativePath.stem().
            withSuffix("_binding").
            withExtension(isHeader ? "h" : "cpp");
}

utils::Path filePath(const TypeInfo& typeInfo, bool isHeader)
{
    return filePath(typeInfo.idl, isHeader);
}

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
