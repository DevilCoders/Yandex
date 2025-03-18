#import <YandexMapsMobile/YRTExport.h>

#import <Foundation/Foundation.h>

@class YTDObjcInterface;
@class YTDResponse;
@class YTDStruct;

/**
 * Undocumented
 */
typedef NS_ENUM(NSUInteger, YTDTestError) {
    /**
     * Undocumented
     */
    YTDTestErrorServer,
    /**
     * Undocumented
     */
    YTDTestErrorClient
};

/**
 * Undocumented
 */
typedef void(^YTDResponseHandler)(
    YTDResponse * _Nullable response,
    YTDTestError error);

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDStruct : NSObject

/**
 * Undocumented
 */
@property (nonatomic, readonly) NSInteger i;


+ (nonnull YTDStruct *)structWithI:( NSInteger)i;


@end

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDVariant : NSObject

@property (nonatomic, readonly, nullable) NSNumber *i;

@property (nonatomic, readonly, nullable) NSNumber *f;

+ (nonnull YTDVariant *)variantWithI:(NSInteger)i;

+ (nonnull YTDVariant *)variantWithF:(float)f;

@end


/**
 * Undocumented
 */
YRT_EXPORT @interface YTDResponse : NSObject

/**
 * Undocumented
 */
@property (nonatomic, readonly) NSInteger i;

/**
 * Undocumented
 */
@property (nonatomic, readonly) float f;


+ (nonnull YTDResponse *)responseWithI:( NSInteger)i
                                     f:( float)f;


@end

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDOptionsStructure : NSObject

/**
 * Undocumented
 */
@property (nonatomic, copy, nonnull) NSString *empty;

/**
 * Undocumented
 */
@property (nonatomic, copy, nonnull) NSString *filled;

+ (nonnull YTDOptionsStructure *)optionsStructureWithEmpty:(nonnull NSString *)empty
                                                    filled:(nonnull NSString *)filled;


- (nonnull YTDOptionsStructure *)init;

@end

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDDefaultValue : NSObject

/**
 * Undocumented
 */
@property (nonatomic, readonly, nonnull) NSString *filled;


+ (nonnull YTDDefaultValue *)defaultValueWithFilled:(nonnull NSString *)filled;


- (nonnull YTDDefaultValue *)init;

@end

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDCombinedValues : NSObject

/**
 * Undocumented
 */
@property (nonatomic, readonly, nonnull) NSString *empty;

/**
 * Undocumented
 */
@property (nonatomic, readonly, nonnull) NSString *filled;


+ (nonnull YTDCombinedValues *)combinedValuesWithEmpty:(nonnull NSString *)empty
                                                filled:(nonnull NSString *)filled;


- (nonnull YTDCombinedValues *)init;

@end

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDDefaultTimeintervalValue : NSObject

/**
 * Undocumented
 */
@property (nonatomic, readonly) NSTimeInterval empty;

/**
 * Undocumented
 */
@property (nonatomic, readonly) NSTimeInterval filled;


+ (nonnull YTDDefaultTimeintervalValue *)defaultTimeintervalValueWithEmpty:( NSTimeInterval)empty
                                                                    filled:( NSTimeInterval)filled;


- (nonnull YTDDefaultTimeintervalValue *)init;

@end

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDObjcInterface : NSObject

/**
 * Undocumented
 */
- (void)methodWithIntValue:(NSInteger)intValue
                floatValue:(float)floatValue
                someStruct:(nonnull YTDStruct *)someStruct
                andVariant:(nonnull YTDVariant *)andVariant;

/**
 * Tells if this object is valid or no. Any method called on an invalid
 * object will throw an exception. The object becomes invalid only on UI
 * thread, and only when its implementation depends on objects already
 * destroyed by now. Please refer to general docs about the interface for
 * details on its invalidation.
 */
@property (nonatomic, readonly, getter=isValid) BOOL valid;

@end

/**
 * Undocumented
 */
YRT_EXPORT @interface YTDInterfaceWithDocs : NSObject

/**
 * Link to YTDStruct, to YTDVariant::f, and some unsupported tag
 * {@some.unsupported.tag}.
 *
 * More links after separator: YTDStruct,
 * YTDObjcInterface::methodWithIntValue:floatValue:someStruct:andVariant:,
 * and link to self: methodWithDocsWithI:s:responseHandler:.
 *
 * @param i - YTDObjcInterface, does something important
 * @param s - some struct
 *
 * @return true if successful, false - otherwise
 */
- (BOOL)methodWithDocsWithI:(nonnull YTDObjcInterface *)i
                          s:(nonnull YTDStruct *)s
            responseHandler:(nonnull YTDResponseHandler)responseHandler;

@end
