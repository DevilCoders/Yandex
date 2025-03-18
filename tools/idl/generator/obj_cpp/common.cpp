#include "obj_cpp/common.h"

#include "common/common.h"

#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/targets.h>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

namespace {

utils::Path filePathFromTypeName(
    const std::string& frameworkName,
    const std::string& typeName,
    bool isHeader)
{
    return frameworkName + "/" + (isHeader ? "Internal/" : "") +
        typeName + (isHeader ? "_Private.h" : "_Binding.mm");
}

} // namespace

utils::Path filePath(const Idl* idl, bool isHeader)
{
    auto prefix = idl->objcTypePrefix.asPrefix("");
    std::string suffix = idl->relativePath.stem().camelCased(true);

    return filePathFromTypeName(
        idl->objcFramework, prefix + suffix, isHeader);
}

utils::Path filePath(
    const Idl* idl,
    const nodes::Name& topLevelTypeName,
    bool isHeader)
{
    auto prefix = idl->objcTypePrefix.asPrefix("");

    return filePathFromTypeName(
        idl->objcFramework, prefix + topLevelTypeName[OBJC], isHeader);
}

utils::Path filePath(const TypeInfo& typeInfo, bool isHeader)
{
    auto prefix = typeInfo.idl->objcTypePrefix.asPrefix("");

    std::string suffix;
    if (typeInfo.scope.original().isEmpty()) {
        auto typePathStem = typeInfo.idl->relativePath.stem();
        suffix = utils::toCamelCase(typePathStem, true);
    } else {
        suffix = typeInfo.scope[OBJC].first();
    }

    return filePathFromTypeName(
        typeInfo.idl->objcFramework, prefix + suffix, isHeader);
}

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
