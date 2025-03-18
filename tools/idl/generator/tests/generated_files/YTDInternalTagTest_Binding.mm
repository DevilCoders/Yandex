#import <YandexMapKit/Internal/YTDInternalTagTest_Private.h>
#import <YandexMapKit/YTDInternalTagTest.h>

#import <YandexMapsMobile/YRTNativeObject.h>

#import <yandex/maps/runtime/any/ios/to_platform.h>
#import <yandex/maps/runtime/bindings/ios/to_native.h>
#import <yandex/maps/runtime/bindings/ios/to_platform.h>
#import <yandex/maps/runtime/ios/exception.h>
#import <yandex/maps/runtime/ios/object.h>
#import <yandex/maps/runtime/verify_and_run.h>

#import <test/docs/internal_tag_test.h>

namespace test {
namespace docs {
namespace ios {

FirstCallback firstCallback(
    YTDSoSecret handler)
{
    return [handler](
        const std::shared_ptr<::test::docs::VeryPrivate>& muchClassified)
    {
        if (!handler) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            handler(
                ::yandex::maps::runtime::bindings::ios::toPlatform(muchClassified));
        });
    };
}


CantMarkMethodsAsInternalHereYet cantMarkMethodsAsInternalHereYet(
    YTDSoDeclassified handler)
{
    return [handler](
        const std::shared_ptr<::test::docs::VeryOpen>& knownStructure)
    {
        if (!handler) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            handler(
                ::yandex::maps::runtime::bindings::ios::toPlatform(knownStructure));
        });
    };
}


OnEmpty onEmpty(
    YTDEmptyCallback handler)
{
    return [handler]()
    {
        if (!handler) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            handler();
        });
    };
}


OnSuccess onSuccess(
    YTDLambdaListenerWithTwoMethods handler)
{
    return [handler]()
    {
        if (!handler) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            handler(
                {});
        });
    };
}
OnError onError(
    YTDLambdaListenerWithTwoMethods handler)
{
    return [handler](
        int error)
    {
        if (!handler) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            handler(
                ::yandex::maps::runtime::bindings::ios::toPlatform(error));
        });
    };
}


OnCallback onCallback(
    YTDCallbackWithParam handler)
{
    return [handler](
        int i)
    {
        if (!handler) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            handler(
                ::yandex::maps::runtime::bindings::ios::toPlatform(i));
        });
    };
}

} // namespace ios
} // namespace docs
} // namespace test

@implementation YTDVeryPrivate {
    YRTNativeObject *_nativeObject;

    NSInteger _regularField;
    BOOL _regularField__is_initialized;

    float _oneMoreRegularField;
    BOOL _oneMoreRegularField__is_initialized;
}

- (YTDVeryPrivate *)initWithNativeObject:(YRTNativeObject*)nativeObject {
    self = [super init];
    if (self) {
        _nativeObject = nativeObject;
        _regularField__is_initialized = NO;
        _oneMoreRegularField__is_initialized = NO;
    }

    return self;
}

+ (YTDVeryPrivate *)veryPrivateWithRegularField:(NSInteger)regularField
                            oneMoreRegularField:(float)oneMoreRegularField
{

    BEGIN_BINDING_METHOD;

    auto nativeSelf = std::make_shared<::test::docs::VeryPrivate>();

    nativeSelf->regularField =
        ::yandex::maps::runtime::bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->regularField)>::type>(regularField);
    nativeSelf->oneMoreRegularField =
        ::yandex::maps::runtime::bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->oneMoreRegularField)>::type>(oneMoreRegularField);

    YTDVeryPrivate *obj = [YTDVeryPrivate alloc];

    if (obj) {
        obj = [obj initWithNativeObject:
            ::yandex::maps::runtime::ios::makeSharedObject<::test::docs::VeryPrivate>(nativeSelf)];

        obj->_regularField = regularField;
        obj->_regularField__is_initialized = YES;
        obj->_oneMoreRegularField = oneMoreRegularField;
        obj->_oneMoreRegularField__is_initialized = YES;
    }

    return obj;

    END_BINDING_METHOD;
    return {};
}

- (NSInteger)regularField
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_regularField__is_initialized) {
            // static_cast here is needed only for enum types.
            _regularField = static_cast<NSInteger>(
                ::yandex::maps::runtime::bindings::ios::toPlatform(
                    ::yandex::maps::runtime::ios::sharedObject_cpp_cast<::test::docs::VeryPrivate>(_nativeObject)->regularField));
            _regularField__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _regularField;
    }
}

- (float)oneMoreRegularField
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_oneMoreRegularField__is_initialized) {
            // static_cast here is needed only for enum types.
            _oneMoreRegularField = static_cast<float>(
                ::yandex::maps::runtime::bindings::ios::toPlatform(
                    ::yandex::maps::runtime::ios::sharedObject_cpp_cast<::test::docs::VeryPrivate>(_nativeObject)->oneMoreRegularField));
            _oneMoreRegularField__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _oneMoreRegularField;
    }
}

- (YRTNativeObject*)getNativeObject {
    return _nativeObject;
}

+ (NSString *)getNativeName
{
    return @"test::docs::VeryPrivate";
}

template boost::any yandex::maps::runtime::any::internal::BridgedHolder<::test::docs::VeryPrivate>::platformObject() const;

@end

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace ios {
namespace internal {

::test::docs::SuchHidden ToNative<::test::docs::SuchHidden, YTDSuchHidden, void>::from(
    YTDSuchHidden* platformSuchHidden)
{
    return ::test::docs::SuchHidden(
        toNative<float>(platformSuchHidden.regularField),
        toNative<int>(platformSuchHidden.oneMoreRegularField),
        toNative<float>(platformSuchHidden.twoMoreRegularFields));
}

YTDSuchHidden* ToPlatform<::test::docs::SuchHidden>::from(
    const ::test::docs::SuchHidden& nativeSuchHidden)
{
    return [YTDSuchHidden suchHiddenWithRegularField:static_cast<float>(toPlatform(nativeSuchHidden.regularField()))
                                 oneMoreRegularField:static_cast<NSInteger>(toPlatform(nativeSuchHidden.oneMoreRegularField()))
                                twoMoreRegularFields:static_cast<float>(toPlatform(nativeSuchHidden.twoMoreRegularFields()))];
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

::test::docs::SuchOptions ToNative<::test::docs::SuchOptions, YTDSuchOptions, void>::from(
    YTDSuchOptions* platformSuchOptions)
{
    return ::test::docs::SuchOptions(
        toNative<std::unique_ptr<::test::docs::TooInternal>>(platformSuchOptions.interfaceField));
}

YTDSuchOptions* ToPlatform<::test::docs::SuchOptions>::from(
    const ::test::docs::SuchOptions& nativeSuchOptions)
{
    return [YTDSuchOptions suchOptionsWithInterfaceField:static_cast<YTDTooInternal *>(toPlatform(nativeSuchOptions.interfaceField()))];
}

} // namespace internal
} // namespace ios
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex


@implementation YTDTooInternal {
    std::unique_ptr<::test::docs::TooInternal> _native;
}

- (::test::docs::TooInternal *)nativeTooInternal
{
    return ::yandex::maps::runtime::ios::uniqueGet(_native);
}
- (::test::docs::TooInternal *)native
{
    return [self nativeTooInternal];
}

- (id)init
{
    NSAssert(NO, @"Don't call init on YTDTooInternal - it's a binding implementation! Use factory instead.");
    return nil;
}

- (void)deinitialize
{
   _native.reset();
}

- (id)initWithWrappedNative:(NSValue *)native
{
    return [self initWithNative:std::move(*static_cast<std::unique_ptr<::test::docs::TooInternal>*>(native.pointerValue))];
}

- (id)initWithNative:(std::unique_ptr<::test::docs::TooInternal>)native
{
    self = [super init];
    if (!self) {
        return nil;
    }

    NSParameterAssert(native);
    _native = std::move(native);
    return self;
}

- (BOOL)regularMethodWithMuchPrivate:(YTDSuchHidden *)muchPrivate
                          soInternal:(YTDVeryPrivate *)soInternal
{
    BEGIN_BINDING_METHOD;

    NSParameterAssert(muchPrivate);

    NSParameterAssert(soInternal);

    return ::yandex::maps::runtime::bindings::ios::toPlatform(self.nativeTooInternal->regularMethod(
        ::yandex::maps::runtime::bindings::ios::toNative<const ::test::docs::SuchHidden>(muchPrivate),
        ::yandex::maps::runtime::bindings::ios::toNative<std::shared_ptr<::test::docs::VeryPrivate>>(soInternal)));

    END_BINDING_METHOD;
    return {};
}

@end



@implementation YTDVeryOpen {
    YRTNativeObject *_nativeObject;

    NSInteger _regularField;
    BOOL _regularField__is_initialized;

    NSNumber *_hiddenSwitch;
    BOOL _hiddenSwitch__is_initialized;
}

- (YTDVeryOpen *)initWithNativeObject:(YRTNativeObject*)nativeObject {
    self = [super init];
    if (self) {
        _nativeObject = nativeObject;
        _regularField__is_initialized = NO;
        _hiddenSwitch__is_initialized = NO;
    }

    return self;
}

+ (YTDVeryOpen *)veryOpenWithRegularField:(NSInteger)regularField
                             hiddenSwitch:(NSNumber *)hiddenSwitch
{

    BEGIN_BINDING_METHOD;

    auto nativeSelf = std::make_shared<::test::docs::VeryOpen>();

    nativeSelf->regularField =
        ::yandex::maps::runtime::bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->regularField)>::type>(regularField);
    nativeSelf->hiddenSwitch =
        ::yandex::maps::runtime::bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->hiddenSwitch)>::type>(hiddenSwitch);

    YTDVeryOpen *obj = [YTDVeryOpen alloc];

    if (obj) {
        obj = [obj initWithNativeObject:
            ::yandex::maps::runtime::ios::makeSharedObject<::test::docs::VeryOpen>(nativeSelf)];

        obj->_regularField = regularField;
        obj->_regularField__is_initialized = YES;
        obj->_hiddenSwitch = hiddenSwitch;
        obj->_hiddenSwitch__is_initialized = YES;
    }

    return obj;

    END_BINDING_METHOD;
    return {};
}

- (NSInteger)regularField
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_regularField__is_initialized) {
            // static_cast here is needed only for enum types.
            _regularField = static_cast<NSInteger>(
                ::yandex::maps::runtime::bindings::ios::toPlatform(
                    ::yandex::maps::runtime::ios::sharedObject_cpp_cast<::test::docs::VeryOpen>(_nativeObject)->regularField));
            _regularField__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _regularField;
    }
}

- (NSNumber *)hiddenSwitch
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_hiddenSwitch__is_initialized) {
            // static_cast here is needed only for enum types.
            _hiddenSwitch = static_cast<NSNumber *>(
                ::yandex::maps::runtime::bindings::ios::toPlatform(
                    ::yandex::maps::runtime::ios::sharedObject_cpp_cast<::test::docs::VeryOpen>(_nativeObject)->hiddenSwitch));
            _hiddenSwitch__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _hiddenSwitch;
    }
}

- (YRTNativeObject*)getNativeObject {
    return _nativeObject;
}

+ (NSString *)getNativeName
{
    return @"test::docs::VeryOpen";
}

template boost::any yandex::maps::runtime::any::internal::BridgedHolder<::test::docs::VeryOpen>::platformObject() const;

@end

@implementation YTDMuchUnprotected {
    YRTNativeObject *_nativeObject;

    float _regularField;
    BOOL _regularField__is_initialized;

    BOOL _oneMoreRegularField;
    BOOL _oneMoreRegularField__is_initialized;

    NSNumber *_hiddenField;
    BOOL _hiddenField__is_initialized;
}

- (YTDMuchUnprotected *)initWithNativeObject:(YRTNativeObject*)nativeObject {
    self = [super init];
    if (self) {
        _nativeObject = nativeObject;
        _regularField__is_initialized = NO;
        _oneMoreRegularField__is_initialized = NO;
        _hiddenField__is_initialized = NO;
    }

    return self;
}

+ (YTDMuchUnprotected *)muchUnprotectedWithRegularField:(float)regularField
                                    oneMoreRegularField:(BOOL)oneMoreRegularField
                                            hiddenField:(NSNumber *)hiddenField
{

    BEGIN_BINDING_METHOD;

    auto nativeSelf = std::make_shared<::test::docs::MuchUnprotected>();

    nativeSelf->regularField =
        ::yandex::maps::runtime::bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->regularField)>::type>(regularField);
    nativeSelf->oneMoreRegularField =
        ::yandex::maps::runtime::bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->oneMoreRegularField)>::type>(oneMoreRegularField);
    nativeSelf->hiddenField =
        ::yandex::maps::runtime::bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->hiddenField)>::type>(hiddenField);

    YTDMuchUnprotected *obj = [YTDMuchUnprotected alloc];

    if (obj) {
        obj = [obj initWithNativeObject:
            ::yandex::maps::runtime::ios::makeSharedObject<::test::docs::MuchUnprotected>(nativeSelf)];

        obj->_regularField = regularField;
        obj->_regularField__is_initialized = YES;
        obj->_oneMoreRegularField = oneMoreRegularField;
        obj->_oneMoreRegularField__is_initialized = YES;
        obj->_hiddenField = hiddenField;
        obj->_hiddenField__is_initialized = YES;
    }

    return obj;

    END_BINDING_METHOD;
    return {};
}

- (float)regularField
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_regularField__is_initialized) {
            // static_cast here is needed only for enum types.
            _regularField = static_cast<float>(
                ::yandex::maps::runtime::bindings::ios::toPlatform(
                    ::yandex::maps::runtime::ios::sharedObject_cpp_cast<::test::docs::MuchUnprotected>(_nativeObject)->regularField));
            _regularField__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _regularField;
    }
}

- (BOOL)oneMoreRegularField
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_oneMoreRegularField__is_initialized) {
            // static_cast here is needed only for enum types.
            _oneMoreRegularField = static_cast<BOOL>(
                ::yandex::maps::runtime::bindings::ios::toPlatform(
                    ::yandex::maps::runtime::ios::sharedObject_cpp_cast<::test::docs::MuchUnprotected>(_nativeObject)->oneMoreRegularField));
            _oneMoreRegularField__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _oneMoreRegularField;
    }
}

- (NSNumber *)hiddenField
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_hiddenField__is_initialized) {
            // static_cast here is needed only for enum types.
            _hiddenField = static_cast<NSNumber *>(
                ::yandex::maps::runtime::bindings::ios::toPlatform(
                    ::yandex::maps::runtime::ios::sharedObject_cpp_cast<::test::docs::MuchUnprotected>(_nativeObject)->hiddenField));
            _hiddenField__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _hiddenField;
    }
}

- (YRTNativeObject*)getNativeObject {
    return _nativeObject;
}

+ (NSString *)getNativeName
{
    return @"test::docs::MuchUnprotected";
}

template boost::any yandex::maps::runtime::any::internal::BridgedHolder<::test::docs::MuchUnprotected>::platformObject() const;

@end

@implementation YTDTooExternal {
    std::unique_ptr<::test::docs::TooExternal> _native;
}

- (::test::docs::TooExternal *)nativeTooExternal
{
    return ::yandex::maps::runtime::ios::uniqueGet(_native);
}
- (::test::docs::TooExternal *)native
{
    return [self nativeTooExternal];
}

- (id)init
{
    NSAssert(NO, @"Don't call init on YTDTooExternal - it's a binding implementation! Use factory instead.");
    return nil;
}

- (void)deinitialize
{
   _native.reset();
}

- (id)initWithWrappedNative:(NSValue *)native
{
    return [self initWithNative:std::move(*static_cast<std::unique_ptr<::test::docs::TooExternal>*>(native.pointerValue))];
}

- (id)initWithNative:(std::unique_ptr<::test::docs::TooExternal>)native
{
    self = [super init];
    if (!self) {
        return nil;
    }

    NSParameterAssert(native);
    _native = std::move(native);
    return self;
}

- (BOOL)regularMethodWithStructure:(YTDMuchUnprotected *)structure
                    soDeclassified:(YTDSoDeclassified)soDeclassified
{
    BEGIN_BINDING_METHOD;

    NSParameterAssert(structure);

    return ::yandex::maps::runtime::bindings::ios::toPlatform(self.nativeTooExternal->regularMethod(
        ::yandex::maps::runtime::bindings::ios::toNative<const std::shared_ptr<::test::docs::MuchUnprotected>>(structure),
        test::docs::ios::cantMarkMethodsAsInternalHereYet([soDeclassified copy])));

    END_BINDING_METHOD;
    return {};
}

- (void)hiddenMethodWithStructure:(YTDSuchHidden *)structure
                         soSecret:(YTDSoSecret)soSecret
{
    BEGIN_BINDING_METHOD;

    NSParameterAssert(structure);

    self.nativeTooExternal->hiddenMethod(
        ::yandex::maps::runtime::bindings::ios::toNative<const ::test::docs::SuchHidden>(structure),
        test::docs::ios::firstCallback([soSecret copy]));

    END_BINDING_METHOD;
}

@end



@implementation YTDWithInternalEnum {
    YRTNativeObject *_nativeObject;

    YTDWithInternalEnumInternalEnum _e;
    BOOL _e__is_initialized;
}

- (YTDWithInternalEnum *)initWithNativeObject:(YRTNativeObject*)nativeObject {
    self = [super init];
    if (self) {
        _nativeObject = nativeObject;
        _e__is_initialized = NO;
    }

    return self;
}

+ (YTDWithInternalEnum *)withInternalEnumWithE:(YTDWithInternalEnumInternalEnum)e
{

    BEGIN_BINDING_METHOD;

    auto nativeSelf = std::make_shared<::test::docs::WithInternalEnum>();

    nativeSelf->e =
        ::yandex::maps::runtime::bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->e)>::type>(e);

    YTDWithInternalEnum *obj = [YTDWithInternalEnum alloc];

    if (obj) {
        obj = [obj initWithNativeObject:
            ::yandex::maps::runtime::ios::makeSharedObject<::test::docs::WithInternalEnum>(nativeSelf)];

        obj->_e = e;
        obj->_e__is_initialized = YES;
    }

    return obj;

    END_BINDING_METHOD;
    return {};
}

- (YTDWithInternalEnumInternalEnum)e
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_e__is_initialized) {
            // static_cast here is needed only for enum types.
            _e = static_cast<YTDWithInternalEnumInternalEnum>(
                ::yandex::maps::runtime::bindings::ios::toPlatform(
                    ::yandex::maps::runtime::ios::sharedObject_cpp_cast<::test::docs::WithInternalEnum>(_nativeObject)->e));
            _e__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _e;
    }
}

- (YRTNativeObject*)getNativeObject {
    return _nativeObject;
}

+ (NSString *)getNativeName
{
    return @"test::docs::WithInternalEnum";
}

template boost::any yandex::maps::runtime::any::internal::BridgedHolder<::test::docs::WithInternalEnum>::platformObject() const;

@end
