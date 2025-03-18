#import <YandexMapKit/Internal/YTDTest_Private.h>
#import <YandexMapKit/YTDTest.h>

#import <YandexMapsMobile/YRTNativeObject.h>

#import <yandex/maps/runtime/any/ios/to_platform.h>
#import <yandex/maps/runtime/bindings/ios/to_native.h>
#import <yandex/maps/runtime/bindings/ios/to_native_fwd.h>
#import <yandex/maps/runtime/bindings/ios/to_platform.h>
#import <yandex/maps/runtime/bindings/ios/to_platform_fwd.h>
#import <yandex/maps/runtime/ios/exception.h>
#import <yandex/maps/runtime/ios/object.h>
#import <yandex/maps/runtime/verify_and_run.h>

#import <test/docs/test.h>
#import <type_traits>

namespace test {
namespace docs {
namespace ios {

OnResponse onResponse(
    YTDResponseHandler handler)
{
    return [handler](
        const std::shared_ptr<::test::docs::Response>& response)
    {
        if (!handler) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            handler(
                ::yandex::maps::runtime::bindings::ios::toPlatform(response),
                {});
        });
    };
}
OnError onError(
    YTDResponseHandler handler)
{
    return [handler](
        ::test::docs::TestError error)
    {
        if (!handler) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            handler(
                {},
                static_cast<YTDTestError>(error));
        });
    };
}

} // namespace ios
} // namespace docs
} // namespace test

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace ios {
namespace internal {

::test::docs::Struct ToNative<::test::docs::Struct, YTDStruct, void>::from(
    YTDStruct* platformStruct)
{
    return ::test::docs::Struct(
        toNative<int>(platformStruct.i));
}

YTDStruct* ToPlatform<::test::docs::Struct>::from(
    const ::test::docs::Struct& nativeStruct)
{
    return [YTDStruct structWithI:static_cast<NSInteger>(toPlatform(nativeStruct.i))];
}

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

::test::docs::Variant ToNative<::test::docs::Variant, id, void>::from(
    id platformVariant)
{
    YTDVariant *typedPlatformVariant = platformVariant;

    if (auto platformI = typedPlatformVariant.i) {
        return ::yandex::maps::runtime::bindings::ios::toNative<int>(platformI);
    }

    if (auto platformF = typedPlatformVariant.f) {
        return ::yandex::maps::runtime::bindings::ios::toNative<float>(platformF);
    }

    throw ::yandex::maps::runtime::Exception("Invalid variant value");
}

id ToPlatform<::test::docs::Variant>::from(
    const ::test::docs::Variant& nativeVariant)
{
    struct ToObjcVisitor : public boost::static_visitor<id> {
        id operator() (const int& nativeVariantObject) const
        {
            return [YTDVariant variantWithI: ::yandex::maps::runtime::bindings::ios::toPlatform(nativeVariantObject)];
        }

        id operator() (const float& nativeVariantObject) const
        {
            return [YTDVariant variantWithF: ::yandex::maps::runtime::bindings::ios::toPlatform(nativeVariantObject)];
        }
    };

    return boost::apply_visitor(ToObjcVisitor(), nativeVariant);
}

} // namespace internal
} // namespace ios
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

@implementation YTDResponse {
    YRTNativeObject *_nativeObject;

    NSInteger _i;
    BOOL _i__is_initialized;

    float _f;
    BOOL _f__is_initialized;
}

- (YTDResponse *)initWithNativeObject:(YRTNativeObject*)nativeObject {
    self = [super init];
    if (self) {
        _nativeObject = nativeObject;
        _i__is_initialized = NO;
        _f__is_initialized = NO;
    }

    return self;
}

+ (YTDResponse *)responseWithI:(NSInteger)i
                             f:(float)f
{

    BEGIN_BINDING_METHOD;

    auto nativeSelf = std::make_shared<::test::docs::Response>();

    nativeSelf->i =
        ::yandex::maps::runtime::bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->i)>::type>(i);
    nativeSelf->f =
        ::yandex::maps::runtime::bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->f)>::type>(f);

    YTDResponse *obj = [YTDResponse alloc];

    if (obj) {
        obj = [obj initWithNativeObject:
            ::yandex::maps::runtime::ios::makeSharedObject<::test::docs::Response>(nativeSelf)];

        obj->_i = i;
        obj->_i__is_initialized = YES;
        obj->_f = f;
        obj->_f__is_initialized = YES;
    }

    return obj;

    END_BINDING_METHOD;
    return {};
}

- (NSInteger)i
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_i__is_initialized) {
            // static_cast here is needed only for enum types.
            _i = static_cast<NSInteger>(
                ::yandex::maps::runtime::bindings::ios::toPlatform(
                    ::yandex::maps::runtime::ios::sharedObject_cpp_cast<::test::docs::Response>(_nativeObject)->i));
            _i__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _i;
    }
}

- (float)f
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_f__is_initialized) {
            // static_cast here is needed only for enum types.
            _f = static_cast<float>(
                ::yandex::maps::runtime::bindings::ios::toPlatform(
                    ::yandex::maps::runtime::ios::sharedObject_cpp_cast<::test::docs::Response>(_nativeObject)->f));
            _f__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _f;
    }
}

- (YRTNativeObject*)getNativeObject {
    return _nativeObject;
}

+ (NSString *)getNativeName
{
    return @"test::docs::Response";
}

template boost::any yandex::maps::runtime::any::internal::BridgedHolder<::test::docs::Response>::platformObject() const;

@end

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace ios {
namespace internal {

::test::docs::OptionsStructure ToNative<::test::docs::OptionsStructure, YTDOptionsStructure, void>::from(
    YTDOptionsStructure* platformOptionsStructure)
{
    return ::test::docs::OptionsStructure(
        toNative<std::string>(platformOptionsStructure.empty),
        toNative<std::string>(platformOptionsStructure.filled));
}

YTDOptionsStructure* ToPlatform<::test::docs::OptionsStructure>::from(
    const ::test::docs::OptionsStructure& nativeOptionsStructure)
{
    return [YTDOptionsStructure optionsStructureWithEmpty:static_cast<NSString *>(toPlatform(nativeOptionsStructure.empty()))
                                                   filled:static_cast<NSString *>(toPlatform(nativeOptionsStructure.filled()))];
}

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

::test::docs::DefaultValue ToNative<::test::docs::DefaultValue, YTDDefaultValue, void>::from(
    YTDDefaultValue* platformDefaultValue)
{
    return ::test::docs::DefaultValue(
        toNative<std::string>(platformDefaultValue.filled));
}

YTDDefaultValue* ToPlatform<::test::docs::DefaultValue>::from(
    const ::test::docs::DefaultValue& nativeDefaultValue)
{
    return [YTDDefaultValue defaultValueWithFilled:static_cast<NSString *>(toPlatform(nativeDefaultValue.filled))];
}

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

::test::docs::CombinedValues ToNative<::test::docs::CombinedValues, YTDCombinedValues, void>::from(
    YTDCombinedValues* platformCombinedValues)
{
    return ::test::docs::CombinedValues(
        toNative<std::string>(platformCombinedValues.empty),
        toNative<std::string>(platformCombinedValues.filled));
}

YTDCombinedValues* ToPlatform<::test::docs::CombinedValues>::from(
    const ::test::docs::CombinedValues& nativeCombinedValues)
{
    return [YTDCombinedValues combinedValuesWithEmpty:static_cast<NSString *>(toPlatform(nativeCombinedValues.empty))
                                               filled:static_cast<NSString *>(toPlatform(nativeCombinedValues.filled))];
}

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

::test::docs::DefaultTimeintervalValue ToNative<::test::docs::DefaultTimeintervalValue, YTDDefaultTimeintervalValue, void>::from(
    YTDDefaultTimeintervalValue* platformDefaultTimeintervalValue)
{
    return ::test::docs::DefaultTimeintervalValue(
        toNative<::yandex::maps::runtime::TimeInterval>(platformDefaultTimeintervalValue.empty),
        toNative<::yandex::maps::runtime::TimeInterval>(platformDefaultTimeintervalValue.filled));
}

YTDDefaultTimeintervalValue* ToPlatform<::test::docs::DefaultTimeintervalValue>::from(
    const ::test::docs::DefaultTimeintervalValue& nativeDefaultTimeintervalValue)
{
    return [YTDDefaultTimeintervalValue defaultTimeintervalValueWithEmpty:static_cast<NSTimeInterval>(toPlatform(nativeDefaultTimeintervalValue.empty))
                                                                   filled:static_cast<NSTimeInterval>(toPlatform(nativeDefaultTimeintervalValue.filled))];
}

} // namespace internal
} // namespace ios
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex


@implementation YTDObjcInterface {
    std::weak_ptr<::test::docs::Interface> _native;
}

- (std::shared_ptr<::test::docs::Interface>)nativeInterface
{
    return ::yandex::maps::runtime::ios::weakGet(_native);
}
- (std::shared_ptr<::test::docs::Interface>)native
{
    return [self nativeInterface];
}

- (id)init
{
    NSAssert(NO, @"Don't call init on YTDObjcInterface - it's a binding implementation! Use factory instead.");
    return nil;
}

- (id)initWithWrappedNative:(NSValue *)native
{
    return [self initWithNative:*static_cast<std::shared_ptr<::test::docs::Interface>*>(native.pointerValue)];
}

- (id)initWithNative:(const std::shared_ptr<::test::docs::Interface>&)native
{
    self = [super init];
    if (!self) {
        return nil;
    }

    NSParameterAssert(native);
    _native = native;
    return self;
}

- (void)methodWithIntValue:(NSInteger)intValue
                floatValue:(float)floatValue
                someStruct:(YTDStruct *)someStruct
                andVariant:(YTDVariant *)andVariant
{
    BEGIN_BINDING_METHOD;

    NSParameterAssert(someStruct);

    NSParameterAssert(andVariant);

    self.nativeInterface->method(
        ::yandex::maps::runtime::bindings::ios::toNative<int>(intValue),
        ::yandex::maps::runtime::bindings::ios::toNative<float>(floatValue),
        ::yandex::maps::runtime::bindings::ios::toNative<const ::test::docs::Struct>(someStruct),
        ::yandex::maps::runtime::bindings::ios::toNative<::test::docs::Variant>(andVariant));

    END_BINDING_METHOD;
}

- (BOOL)isValid
{
    return !_native.expired();
}

@end

namespace test {
namespace docs {

boost::any createPlatform(const std::shared_ptr<::test::docs::Interface>& interface)
{
    return (id)[[YTDObjcInterface alloc] initWithNative:interface];
}

} // namespace docs
} // namespace test


@implementation YTDInterfaceWithDocs {
    std::unique_ptr<::test::docs::InterfaceWithDocs> _native;
}

- (::test::docs::InterfaceWithDocs *)nativeInterfaceWithDocs
{
    return ::yandex::maps::runtime::ios::uniqueGet(_native);
}
- (::test::docs::InterfaceWithDocs *)native
{
    return [self nativeInterfaceWithDocs];
}

- (id)init
{
    NSAssert(NO, @"Don't call init on YTDInterfaceWithDocs - it's a binding implementation! Use factory instead.");
    return nil;
}

- (void)deinitialize
{
   _native.reset();
}

- (id)initWithWrappedNative:(NSValue *)native
{
    return [self initWithNative:std::move(*static_cast<std::unique_ptr<::test::docs::InterfaceWithDocs>*>(native.pointerValue))];
}

- (id)initWithNative:(std::unique_ptr<::test::docs::InterfaceWithDocs>)native
{
    self = [super init];
    if (!self) {
        return nil;
    }

    NSParameterAssert(native);
    _native = std::move(native);
    return self;
}

- (BOOL)methodWithDocsWithI:(YTDObjcInterface *)i
                          s:(YTDStruct *)s
            responseHandler:(YTDResponseHandler)responseHandler
{
    BEGIN_BINDING_METHOD;

    NSParameterAssert(s);

    return ::yandex::maps::runtime::bindings::ios::toPlatform(self.nativeInterfaceWithDocs->methodWithDocs(
        ::yandex::maps::runtime::bindings::ios::toNative<::test::docs::Interface*>(i),
        ::yandex::maps::runtime::bindings::ios::toNative<const ::test::docs::Struct>(s),
        test::docs::ios::onResponse([responseHandler copy]),
        test::docs::ios::onError([responseHandler copy])));

    END_BINDING_METHOD;
    return {};
}

@end


