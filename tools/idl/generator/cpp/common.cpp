#include "cpp/common.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

utils::Path filePath(const Idl* idl, bool isHeader)
{
    return idl->cppNamespace.asPath() +
        idl->relativePath.stem().withExtension(isHeader ? "h" : "cpp");
}

utils::Path filePath(const TypeInfo& typeInfo, bool isHeader)
{
    return filePath(typeInfo.idl, isHeader);
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
