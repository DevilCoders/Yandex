#include "protoconv/common.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

utils::Path filePath(const Idl* idl, bool isHeader)
{
    return idl->cppNamespace.asPath() +
        idl->relativePath.stem().withExtension(isHeader ? "conv.h" : "conv.cpp");
}

utils::Path filePath(const TypeInfo& typeInfo, bool isHeader)
{
    return filePath(typeInfo.idl, isHeader);
}

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
