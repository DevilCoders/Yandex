#pragma once

#include <yandex/maps/runtime/android/object.h>
#include <yandex/maps/runtime/any/android/to_platform.h>
#include <yandex/maps/runtime/bindings/android/internal/new_serialization.h>
#include <yandex/maps/runtime/bindings/android/to_native.h>
#include <yandex/maps/runtime/bindings/android/to_platform.h>
#include <yandex/maps/runtime/exception.h>
#include <yandex/maps/runtime/verify_and_run.h>

#include <test/docs/internal_tag_test.h>

#include <boost/optional.hpp>

#include <functional>
#include <memory>

namespace test {
namespace docs {
namespace android {

YANDEX_EXPORT FirstCallback createFirstCallback(
    ::yandex::maps::runtime::android::JniObject soSecret);

} // namespace android
} // namespace docs
} // namespace test

namespace test {
namespace docs {
namespace android {

YANDEX_EXPORT CantMarkMethodsAsInternalHereYet createCantMarkMethodsAsInternalHereYet(
    ::yandex::maps::runtime::android::JniObject soDeclassified);

} // namespace android
} // namespace docs
} // namespace test

namespace test {
namespace docs {
namespace android {

YANDEX_EXPORT OnEmpty createOnEmpty(
    ::yandex::maps::runtime::android::JniObject emptyCallback);

} // namespace android
} // namespace docs
} // namespace test

namespace test {
namespace docs {
namespace android {

YANDEX_EXPORT OnSuccess createOnSuccess(
    ::yandex::maps::runtime::android::JniObject lambdaListenerWithTwoMethods);

YANDEX_EXPORT OnError createOnError(
    ::yandex::maps::runtime::android::JniObject lambdaListenerWithTwoMethods);

} // namespace android
} // namespace docs
} // namespace test

namespace test {
namespace docs {
namespace android {

YANDEX_EXPORT OnCallback createOnCallback(
    ::yandex::maps::runtime::android::JniObject callbackWithParam);

} // namespace android
} // namespace docs
} // namespace test

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace android {
namespace internal {

template <>
struct ToNative<::test::docs::SuchHidden, jobject, void> {
    static ::test::docs::SuchHidden from(
        jobject platformSuchHidden);
};
template <>
struct ToNative<::test::docs::SuchHidden, ::yandex::maps::runtime::android::JniObject> {
    static ::test::docs::SuchHidden from(
        ::yandex::maps::runtime::android::JniObject platformSuchHidden)
    {
        return ToNative<::test::docs::SuchHidden, jobject>::from(
            platformSuchHidden.get());
    }
};

template <>
struct ToPlatform<::test::docs::SuchHidden> {
    static JniObject from(
        const ::test::docs::SuchHidden& nativeSuchHidden);
};

} // namespace internal
} // namespace android
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex
namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace android {
namespace internal {

template <>
struct ToNative<::test::docs::SuchOptions, jobject, void> {
    static ::test::docs::SuchOptions from(
        jobject platformSuchOptions);
};
template <>
struct ToNative<::test::docs::SuchOptions, ::yandex::maps::runtime::android::JniObject> {
    static ::test::docs::SuchOptions from(
        ::yandex::maps::runtime::android::JniObject platformSuchOptions)
    {
        return ToNative<::test::docs::SuchOptions, jobject>::from(
            platformSuchOptions.get());
    }
};

template <>
struct ToPlatform<::test::docs::SuchOptions> {
    static JniObject from(
        const ::test::docs::SuchOptions& nativeSuchOptions);
};

} // namespace internal
} // namespace android
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex
