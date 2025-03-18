#import <YandexMapKit/YTDInternalTagTest.h>

#import <yandex/maps/runtime/bindings/ios/to_native.h>
#import <yandex/maps/runtime/bindings/ios/to_platform.h>

#import <memory>
#import <test/docs/internal_tag_test.h>

namespace test {
namespace docs {
namespace ios {

FirstCallback firstCallback(
    YTDSoSecret handler);

CantMarkMethodsAsInternalHereYet cantMarkMethodsAsInternalHereYet(
    YTDSoDeclassified handler);

OnEmpty onEmpty(
    YTDEmptyCallback handler);

OnSuccess onSuccess(
    YTDLambdaListenerWithTwoMethods handler);
OnError onError(
    YTDLambdaListenerWithTwoMethods handler);

OnCallback onCallback(
    YTDCallbackWithParam handler);

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
struct ToNative<::test::docs::SuchHidden, YTDSuchHidden, void> {
    static ::test::docs::SuchHidden from(
        YTDSuchHidden* platformSuchHidden);
};

template <typename PlatformType>
struct ToNative<::test::docs::SuchHidden, PlatformType,
        typename std::enable_if<
            std::is_convertible<PlatformType, YTDSuchHidden*>::value>::type> {
    static ::test::docs::SuchHidden from(
        PlatformType platformSuchHidden)
    {
        return ToNative<::test::docs::SuchHidden, YTDSuchHidden>::from(
            platformSuchHidden);
    }
};

template <>
struct ToPlatform<::test::docs::SuchHidden> {
    static YTDSuchHidden* from(
        const ::test::docs::SuchHidden& suchHidden);
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
struct ToNative<::test::docs::SuchOptions, YTDSuchOptions, void> {
    static ::test::docs::SuchOptions from(
        YTDSuchOptions* platformSuchOptions);
};

template <typename PlatformType>
struct ToNative<::test::docs::SuchOptions, PlatformType,
        typename std::enable_if<
            std::is_convertible<PlatformType, YTDSuchOptions*>::value>::type> {
    static ::test::docs::SuchOptions from(
        PlatformType platformSuchOptions)
    {
        return ToNative<::test::docs::SuchOptions, YTDSuchOptions>::from(
            platformSuchOptions);
    }
};

template <>
struct ToPlatform<::test::docs::SuchOptions> {
    static YTDSuchOptions* from(
        const ::test::docs::SuchOptions& suchOptions);
};

} // namespace internal
} // namespace ios
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

@interface YTDTooInternal ()

- (id)initWithWrappedNative:(NSValue *)native;
- (id)initWithNative:(std::unique_ptr<::test::docs::TooInternal>)native;

- (::test::docs::TooInternal *)nativeTooInternal;

@end





@interface YTDTooExternal ()

- (id)initWithWrappedNative:(NSValue *)native;
- (id)initWithNative:(std::unique_ptr<::test::docs::TooExternal>)native;

- (::test::docs::TooExternal *)nativeTooExternal;

@end


