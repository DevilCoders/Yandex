#pragma once

#include <yandex/maps/idl/env.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/type_info.h>
#include <yandex/maps/idl/utils/paths.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {

struct Idl {
    Idl(
        Environment* environment,
        utils::Path relativepath,
        nodes::Root nodesroot,
        std::string bundleguard,
        Scope idlnamespace,
        Scope cppnamespace,
        Scope csnamespace,
        Scope javapackage,
        std::string objcframework,
        Scope objctypeprefix
    )
        : env(environment)
        , relativePath(std::move(relativepath))
        , root(std::move(nodesroot))
        , bundleGuard(std::move(bundleguard))
        , idlNamespace(std::move(idlnamespace))
        , cppNamespace(std::move(cppnamespace))
        , csNamespace(std::move(csnamespace))
        , javaPackage(std::move(javapackage))
        , objcFramework(std::move(objcframework))
        , objcTypePrefix(std::move(objctypeprefix))
    {
    }

    Idl(Idl&& /* source */) = default; // The only constructor we need

    Environment* env;

    utils::Path relativePath;   // "mapkit/search/session.idl"
    nodes::Root root;

    std::string bundleGuard;   // "YANDEX_MAP_KIT"

    Scope idlNamespace;        // { "mapkit", "search" }

    Scope cppNamespace;        // { "yandex", "maps", "mapkit", "search" }
    Scope csNamespace;         // { "Yandex", "MapKit", "Search }
    Scope javaPackage;         // { "com", "yandex", "mapkit", "search" }
    std::string objcFramework; // "YandexMapKit"

    Scope objcTypePrefix;      // { "YMK", "Search" }

    /**
     * Helper method whose purpose is to allow "idl->type(scope, name)"
     * instead of "idl->env->type(idl, scope, name)".
     */
    const TypeInfo& type(
        const Scope& scope,
        const Scope& qualifiedName) const
    {
        return env->type(this, scope, qualifiedName);
    }
};

struct FullScope {
    const Idl* idl;
    Scope scope;

    template <typename ScopeType>
    const TypeInfo& type(const ScopeType& qualifiedName) const
    {
        return idl->type(scope, Scope(qualifiedName));
    }

    template <typename ScopeType>
    FullScope& operator+=(const ScopeType& scope)
    {
        this->scope += scope;
        return *this;
    }
};

template <typename ScopeType>
FullScope operator+(const FullScope& fullScope, const ScopeType& scope)
{
    auto result = fullScope;
    result += scope;
    return result;
}

} // namespace idl
} // namespace maps
} // namespace yandex
