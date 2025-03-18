#pragma once

#include <yandex/maps/runtime/android/object.h>
#include <yandex/maps/runtime/any/android/to_platform.h>
#include <yandex/maps/runtime/bindings/android/internal/new_serialization.h>
#include <yandex/maps/runtime/bindings/android/to_native.h>
#include <yandex/maps/runtime/bindings/android/to_platform.h>
#include <yandex/maps/runtime/exception.h>
#include <yandex/maps/runtime/platform_holder.h>
#include <yandex/maps/runtime/time.h>
#include <yandex/maps/runtime/verify_and_run.h>

#include <test/docs/test.h>

#include <boost/any.hpp>

#include <functional>
#include <memory>
#include <string>

namespace test {
namespace docs {
namespace android {

YANDEX_EXPORT OnResponse createOnResponse(
    ::yandex::maps::runtime::android::JniObject lambdaListener);

YANDEX_EXPORT OnError createOnError(
    ::yandex::maps::runtime::android::JniObject lambdaListener);

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
struct ToNative<::test::docs::Variant, jobject, void> {
    static ::test::docs::Variant from(
        jobject platformVariant);
};
template <>
struct ToNative<::test::docs::Variant, ::yandex::maps::runtime::android::JniObject, void> {
    static ::test::docs::Variant from(
        ::yandex::maps::runtime::android::JniObject platformVariant)
    {
        return ToNative<::test::docs::Variant, jobject>::from(
            platformVariant.get());
    }
};

template <>
struct ToPlatform<::test::docs::Variant> {
    static ::yandex::maps::runtime::android::JniObject from(
        const ::test::docs::Variant& variant);
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
struct ToNative<::test::docs::Struct, jobject, void> {
    static ::test::docs::Struct from(
        jobject platformStruct);
};
template <>
struct ToNative<::test::docs::Struct, ::yandex::maps::runtime::android::JniObject> {
    static ::test::docs::Struct from(
        ::yandex::maps::runtime::android::JniObject platformStruct)
    {
        return ToNative<::test::docs::Struct, jobject>::from(
            platformStruct.get());
    }
};

template <>
struct ToPlatform<::test::docs::Struct> {
    static JniObject from(
        const ::test::docs::Struct& nativeStruct);
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
struct ToNative<::test::docs::OptionsStructure, jobject, void> {
    static ::test::docs::OptionsStructure from(
        jobject platformOptionsStructure);
};
template <>
struct ToNative<::test::docs::OptionsStructure, ::yandex::maps::runtime::android::JniObject> {
    static ::test::docs::OptionsStructure from(
        ::yandex::maps::runtime::android::JniObject platformOptionsStructure)
    {
        return ToNative<::test::docs::OptionsStructure, jobject>::from(
            platformOptionsStructure.get());
    }
};

template <>
struct ToPlatform<::test::docs::OptionsStructure> {
    static JniObject from(
        const ::test::docs::OptionsStructure& nativeOptionsStructure);
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
struct ToNative<::test::docs::DefaultValue, jobject, void> {
    static ::test::docs::DefaultValue from(
        jobject platformDefaultValue);
};
template <>
struct ToNative<::test::docs::DefaultValue, ::yandex::maps::runtime::android::JniObject> {
    static ::test::docs::DefaultValue from(
        ::yandex::maps::runtime::android::JniObject platformDefaultValue)
    {
        return ToNative<::test::docs::DefaultValue, jobject>::from(
            platformDefaultValue.get());
    }
};

template <>
struct ToPlatform<::test::docs::DefaultValue> {
    static JniObject from(
        const ::test::docs::DefaultValue& nativeDefaultValue);
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
struct ToNative<::test::docs::CombinedValues, jobject, void> {
    static ::test::docs::CombinedValues from(
        jobject platformCombinedValues);
};
template <>
struct ToNative<::test::docs::CombinedValues, ::yandex::maps::runtime::android::JniObject> {
    static ::test::docs::CombinedValues from(
        ::yandex::maps::runtime::android::JniObject platformCombinedValues)
    {
        return ToNative<::test::docs::CombinedValues, jobject>::from(
            platformCombinedValues.get());
    }
};

template <>
struct ToPlatform<::test::docs::CombinedValues> {
    static JniObject from(
        const ::test::docs::CombinedValues& nativeCombinedValues);
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
struct ToNative<::test::docs::DefaultTimeintervalValue, jobject, void> {
    static ::test::docs::DefaultTimeintervalValue from(
        jobject platformDefaultTimeintervalValue);
};
template <>
struct ToNative<::test::docs::DefaultTimeintervalValue, ::yandex::maps::runtime::android::JniObject> {
    static ::test::docs::DefaultTimeintervalValue from(
        ::yandex::maps::runtime::android::JniObject platformDefaultTimeintervalValue)
    {
        return ToNative<::test::docs::DefaultTimeintervalValue, jobject>::from(
            platformDefaultTimeintervalValue.get());
    }
};

template <>
struct ToPlatform<::test::docs::DefaultTimeintervalValue> {
    static JniObject from(
        const ::test::docs::DefaultTimeintervalValue& nativeDefaultTimeintervalValue);
};

} // namespace internal
} // namespace android
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex
