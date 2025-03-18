#pragma once

#include <yandex/maps/idl/cache.h>
#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/type_info.h>
#include <yandex/maps/idl/utils/paths.h>

#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

namespace yandex {
namespace maps {
namespace idl {

/**
 * Holds application state. Replacement for global variables.
 */
class Environment {
public:
    Environment(Config config);

    const Config config;

    const Framework* runtimeFramework() const { return runtimeFramework_; }

    const Idl* idl(const utils::Path& relativePath);

    const TypeInfo& type(
        const Idl* idl,
        const Scope& scope,
        const Scope& qualifiedName) const;

private:
    const Framework* getFramework(const utils::Path& relativePath);

    std::unique_ptr<const Idl> buildIdl(
        const utils::Path& searchPath,
        const utils::Path& relativePath,
        const std::string& fileContents);

    void registerTypes(const Idl* idl);

    Cache<const Framework> frameworks_;
    const Framework* const runtimeFramework_;

    Cache<const Idl> idls_;

    std::unordered_map<std::size_t, TypeInfo> types_;
    std::map<std::string, std::set<std::string>> duplicateTypes_;
};

} // namespace idl
} // namespace maps
} // namespace yandex
