#import <YandexMapKit/YTDTest.h>

#import <yandex/maps/runtime/bindings/ios/to_native.h>
#import <yandex/maps/runtime/bindings/ios/to_native_fwd.h>
#import <yandex/maps/runtime/bindings/ios/to_platform.h>
#import <yandex/maps/runtime/bindings/ios/to_platform_fwd.h>

#import <memory>
#import <test/docs/test.h>
#import <type_traits>

namespace test {
namespace docs {
namespace ios {

OnResponse onResponse(
    YTDResponseHandler handler);
OnError onError(
    YTDResponseHandler handler);

} // namespace ios
} // namespace docs
} // namespace test

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace ios {
namespace internal {

template <>
struct ToNative<::test::docs::Struct, YTDStruct, void> {
    static ::test::docs::Struct from(
        YTDStruct* platformStruct);
};

template <typename PlatformType>
struct ToNative<::test::docs::Struct, PlatformType,
        typename std::enable_if<
            std::is_convertible<PlatformType, YTDStruct*>::value>::type> {
    static ::test::docs::Struct from(
        PlatformType platformStruct)
    {
        return ToNative<::test::docs::Struct, YTDStruct>::from(
            platformStruct);
    }
};

template <>
struct ToPlatform<::test::docs::Struct> {
    static YTDStruct* from(
        const ::test::docs::Struct& struct);
};

} // namespace internal
} // namespace ios
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace ios {
namespace internal {

template <>
struct ToNative<::test::docs::Variant, id, void> {
    static ::test::docs::Variant from(
        id platformVariant);
};

template <typename PlatformType>
struct ToNative<::test::docs::Variant, PlatformType,
        typename std::enable_if<
            std::is_convertible<PlatformType, id>::value>::type> {
    static ::test::docs::Variant from(
        PlatformType platformVariant)
    {
        return ToNative<::test::docs::Variant, id>::from(
            platformVariant);
    }
};

template <>
struct ToPlatform<::test::docs::Variant> {
    static id from(
        const ::test::docs::Variant& variant);
};

} // namespace internal
} // namespace ios
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex



namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace ios {
namespace internal {

template <>
struct ToNative<::test::docs::OptionsStructure, YTDOptionsStructure, void> {
    static ::test::docs::OptionsStructure from(
        YTDOptionsStructure* platformOptionsStructure);
};

template <typename PlatformType>
struct ToNative<::test::docs::OptionsStructure, PlatformType,
        typename std::enable_if<
            std::is_convertible<PlatformType, YTDOptionsStructure*>::value>::type> {
    static ::test::docs::OptionsStructure from(
        PlatformType platformOptionsStructure)
    {
        return ToNative<::test::docs::OptionsStructure, YTDOptionsStructure>::from(
            platformOptionsStructure);
    }
};

template <>
struct ToPlatform<::test::docs::OptionsStructure> {
    static YTDOptionsStructure* from(
        const ::test::docs::OptionsStructure& optionsStructure);
};

} // namespace internal
} // namespace ios
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace ios {
namespace internal {

template <>
struct ToNative<::test::docs::DefaultValue, YTDDefaultValue, void> {
    static ::test::docs::DefaultValue from(
        YTDDefaultValue* platformDefaultValue);
};

template <typename PlatformType>
struct ToNative<::test::docs::DefaultValue, PlatformType,
        typename std::enable_if<
            std::is_convertible<PlatformType, YTDDefaultValue*>::value>::type> {
    static ::test::docs::DefaultValue from(
        PlatformType platformDefaultValue)
    {
        return ToNative<::test::docs::DefaultValue, YTDDefaultValue>::from(
            platformDefaultValue);
    }
};

template <>
struct ToPlatform<::test::docs::DefaultValue> {
    static YTDDefaultValue* from(
        const ::test::docs::DefaultValue& defaultValue);
};

} // namespace internal
} // namespace ios
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace ios {
namespace internal {

template <>
struct ToNative<::test::docs::CombinedValues, YTDCombinedValues, void> {
    static ::test::docs::CombinedValues from(
        YTDCombinedValues* platformCombinedValues);
};

template <typename PlatformType>
struct ToNative<::test::docs::CombinedValues, PlatformType,
        typename std::enable_if<
            std::is_convertible<PlatformType, YTDCombinedValues*>::value>::type> {
    static ::test::docs::CombinedValues from(
        PlatformType platformCombinedValues)
    {
        return ToNative<::test::docs::CombinedValues, YTDCombinedValues>::from(
            platformCombinedValues);
    }
};

template <>
struct ToPlatform<::test::docs::CombinedValues> {
    static YTDCombinedValues* from(
        const ::test::docs::CombinedValues& combinedValues);
};

} // namespace internal
} // namespace ios
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace ios {
namespace internal {

template <>
struct ToNative<::test::docs::DefaultTimeintervalValue, YTDDefaultTimeintervalValue, void> {
    static ::test::docs::DefaultTimeintervalValue from(
        YTDDefaultTimeintervalValue* platformDefaultTimeintervalValue);
};

template <typename PlatformType>
struct ToNative<::test::docs::DefaultTimeintervalValue, PlatformType,
        typename std::enable_if<
            std::is_convertible<PlatformType, YTDDefaultTimeintervalValue*>::value>::type> {
    static ::test::docs::DefaultTimeintervalValue from(
        PlatformType platformDefaultTimeintervalValue)
    {
        return ToNative<::test::docs::DefaultTimeintervalValue, YTDDefaultTimeintervalValue>::from(
            platformDefaultTimeintervalValue);
    }
};

template <>
struct ToPlatform<::test::docs::DefaultTimeintervalValue> {
    static YTDDefaultTimeintervalValue* from(
        const ::test::docs::DefaultTimeintervalValue& defaultTimeintervalValue);
};

} // namespace internal
} // namespace ios
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

@interface YTDObjcInterface ()

- (id)initWithWrappedNative:(NSValue *)native;
- (id)initWithNative:(const std::shared_ptr<::test::docs::Interface>&)native;

- (std::shared_ptr<::test::docs::Interface>)nativeInterface;
- (std::shared_ptr<::test::docs::Interface>)native;

@end

@interface YTDInterfaceWithDocs ()

- (id)initWithWrappedNative:(NSValue *)native;
- (id)initWithNative:(std::unique_ptr<::test::docs::InterfaceWithDocs>)native;

- (::test::docs::InterfaceWithDocs *)nativeInterfaceWithDocs;

@end
