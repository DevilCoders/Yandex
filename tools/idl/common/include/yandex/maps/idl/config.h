#pragma once

#include <yandex/maps/idl/utils/paths.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {

struct Config {
    utils::Path inProtoRoot;
    utils::SearchPaths inFrameworkSearchPaths;
    utils::SearchPaths inIdlSearchPaths;

    std::string baseProtoPackage;

    utils::Path outBaseHeadersRoot;
    utils::Path outBaseImplRoot;
    utils::Path outAndroidHeadersRoot;
    utils::Path outAndroidImplRoot;
    utils::Path outIosHeadersRoot;
    utils::Path outIosImplRoot;

    bool disableInternalChecks;
    bool isPublic;
    bool useStdOptional;
};

} // namespace idl
} // namespace maps
} // namespace yandex
