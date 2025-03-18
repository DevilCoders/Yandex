#import <YandexMapsMobile/YRTExport.h>

#import <Foundation/Foundation.h>

@class YTDMuchUnprotected;
@class YTDSuchHidden;
@class YTDVeryOpen;
@class YTDVeryPrivate;

/**
 * :nodoc:
 * This listener should be excluded from documentation.
 */
typedef void(^YTDSoSecret)(
    YTDVeryPrivate * _Nonnull muchClassified);

/**
 * Undocumented
 */
typedef void(^YTDSoDeclassified)(
    YTDVeryOpen * _Nonnull knownStructure);

/**
 * Undocumented
 */
typedef void(^YTDEmptyCallback)(
    void);

/**
 * Undocumented
 */
typedef void(^YTDLambdaListenerWithTwoMethods)(
    NSInteger error);

/**
 * Undocumented
 */
typedef void(^YTDCallbackWithParam)(
    NSInteger i);

/**
 * :nodoc:
 * This struct should be excluded from documentation.
 */
YRT_EXPORT @interface YTDVeryPrivate : NSObject

/**
 * Undocumented
 */
@property (nonatomic, readonly) NSInteger regularField;

/**
 * Undocumented
 */
@property (nonatomic, readonly) float oneMoreRegularField;


+ (nonnull YTDVeryPrivate *)veryPrivateWithRegularField:( NSInteger)regularField
                                    oneMoreRegularField:( float)oneMoreRegularField;


@end

/**
 * :nodoc:
 * This struct should be excluded from documentation.
 */
YRT_EXPORT @interface YTDSuchHidden : NSObject

/**
 * Undocumented
 */
@property (nonatomic, assign) float regularField;

/**
 * Undocumented
 */
@property (nonatomic, assign) NSInteger oneMoreRegularField;

/**
 * Undocumented
 */
@property (nonatomic, assign) float twoMoreRegularFields;

+ (nonnull YTDSuchHidden *)suchHiddenWithRegularField:( float)regularField
                                  oneMoreRegularField:( NSInteger)oneMoreRegularField
                                 twoMoreRegularFields:( float)twoMoreRegularFields;


@end

/**
 * :nodoc:
 * This struct should be excluded from documentation.
 */
YRT_EXPORT @interface YTDSuchOptions : NSObject

/**
 * Undocumented
 */
@property (nonatomic, strong, nonnull) YTDTooInternal *interfaceField;

+ (nonnull YTDSuchOptions *)suchOptionsWithInterfaceField:(nonnull YTDTooInternal *)interfaceField;


@end

/**
 * :nodoc:
 * This interface should be excluded from documentation.
 */
YRT_EXPORT @interface YTDTooInternal : NSObject

/**
 * Undocumented
 */
- (BOOL)regularMethodWithMuchPrivate:(nonnull YTDSuchHidden *)muchPrivate
                          soInternal:(nonnull YTDVeryPrivate *)soInternal;

@end

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDVeryOpen : NSObject

/**
 * Undocumented
 */
@property (nonatomic, readonly) NSInteger regularField;

/**
 * :nodoc:
 * Only this field should be excluded from docs
 *
 * Optional field, can be nil.
 */
@property (nonatomic, readonly, nullable) NSNumber *hiddenSwitch;


+ (nonnull YTDVeryOpen *)veryOpenWithRegularField:( NSInteger)regularField
                                     hiddenSwitch:(nullable NSNumber *)hiddenSwitch;


@end

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDMuchUnprotected : NSObject

/**
 * Undocumented
 */
@property (nonatomic, readonly) float regularField;

/**
 * Undocumented
 */
@property (nonatomic, readonly) BOOL oneMoreRegularField;

/**
 * :nodoc:
 * Only this field should be excluded from docs
 *
 * Optional field, can be nil.
 */
@property (nonatomic, readonly, nullable) NSNumber *hiddenField;


+ (nonnull YTDMuchUnprotected *)muchUnprotectedWithRegularField:( float)regularField
                                            oneMoreRegularField:( BOOL)oneMoreRegularField
                                                    hiddenField:(nullable NSNumber *)hiddenField;


@end

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDTooExternal : NSObject

/**
 * Undocumented
 */
- (BOOL)regularMethodWithStructure:(nonnull YTDMuchUnprotected *)structure
                    soDeclassified:(nonnull YTDSoDeclassified)soDeclassified;

/**
 * :nodoc:
 * Only this method should be excluded from docs
 */
- (void)hiddenMethodWithStructure:(nonnull YTDSuchHidden *)structure
                         soSecret:(nonnull YTDSoSecret)soSecret;

@end

/**
 * Undocumented
 */
typedef NS_ENUM(NSUInteger, YTDWithInternalEnumInternalEnum) {
    /**
     * Undocumented
     */
    YTDWithInternalEnumInternalEnumA,
    /**
     * Undocumented
     */
    YTDWithInternalEnumInternalEnumB,
    /**
     * Undocumented
     */
    YTDWithInternalEnumInternalEnumC
};

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDWithInternalEnum : NSObject

/**
 * Undocumented
 */
@property (nonatomic, readonly) YTDWithInternalEnumInternalEnum e;


+ (nonnull YTDWithInternalEnum *)withInternalEnumWithE:( YTDWithInternalEnumInternalEnum)e;


@end
