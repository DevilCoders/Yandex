#pragma once

#include <yandex/maps/idl/scope.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {

struct Framework {
    Framework(
        std::string bundleguard,
        Scope cppnamespace,
        Scope csnamespace,
        Scope javapackage,
        std::string objcframework,
        Scope objcframeworkprefix
    )
      : bundleGuard(std::move(bundleguard))
      , cppNamespace(std::move(cppnamespace))
      , csNamespace(std::move(csnamespace))
      , javaPackage(std::move(javapackage))
      , objcFramework(std::move(objcframework))
      , objcFrameworkPrefix(std::move(objcframeworkprefix))
    {
    }
    Framework() = default;
    Framework(Framework&& /* source */) = default; // The only c-tor we need

    std::string bundleGuard;   // "YANDEX_MAP_KIT"

    Scope cppNamespace;        // { "yandex", "maps", "mapkit" }
    Scope csNamespace;         // { "Yandex", "MapKit" }
    Scope javaPackage;         // { "com", "yandex", "mapkit" }
    std::string objcFramework; // "YandexMapKit"

    Scope objcFrameworkPrefix; // { "YMK" }
};

} // namespace idl
} // namespace maps
} // namespace yandex
