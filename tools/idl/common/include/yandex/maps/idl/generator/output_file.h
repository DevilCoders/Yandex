#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/utils/paths.h>

#include <set>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {

struct OutputFile {
    OutputFile(
        utils::Path prefixPath,
        utils::Path suffixPath,
        std::string text,
        const std::set<std::string>& imports = { })
        : prefixPath(prefixPath),
          suffixPath(suffixPath),
          text(text),
          imports(imports)
    {
    }

    // Full path (prefix + suffix) is divided in this way to be able to
    // print short path (suffixPath) instead of whole path.
    utils::Path prefixPath;
    utils::Path suffixPath;

    utils::Path fullPath() const { return prefixPath + suffixPath; }

    std::string text;

    std::set<std::string> imports;
};

} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
