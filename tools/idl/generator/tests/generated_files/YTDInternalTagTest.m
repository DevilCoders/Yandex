#import <YandexMapKit/YTDInternalTagTest.h>
#import <YandexMapsMobile/YRTArchivingUtilities.h>
#import <YandexMapsMobile/YRTExport.h>
#import <YandexMapsMobile/YRTSerialization.h>

#pragma mark - YTDVeryPrivate

@interface YTDVeryPrivate () <YRTSerialization>

@end

#pragma mark - YTDSuchHidden

@interface YTDSuchHidden () <YRTSerialization>

@end

@implementation YTDSuchHidden

- (YTDSuchHidden *)initWithRegularField:(float)regularField
                    oneMoreRegularField:(NSInteger)oneMoreRegularField
                   twoMoreRegularFields:(float)twoMoreRegularFields
{
    
    self = [super init];

    if (!self)
        return nil;
    
    _regularField = regularField;
    _oneMoreRegularField = oneMoreRegularField;
    _twoMoreRegularFields = twoMoreRegularFields;

    return self;
}

+ (YTDSuchHidden *)suchHiddenWithRegularField:(float)regularField
                          oneMoreRegularField:(NSInteger)oneMoreRegularField
                         twoMoreRegularFields:(float)twoMoreRegularFields
{
    return [[YTDSuchHidden alloc] initWithRegularField:regularField
                                   oneMoreRegularField:oneMoreRegularField
                                  twoMoreRegularFields:twoMoreRegularFields];
}

#pragma mark - YRTSerialization

- (void)serialize:(id<YRTArchiving>)archive
{
    _regularField = [archive addFloat:_regularField];
    _oneMoreRegularField = [archive addInteger:_oneMoreRegularField];
    _twoMoreRegularFields = [archive addFloat:_twoMoreRegularFields];
}

@end

#pragma mark - YTDSuchOptions

@interface YTDSuchOptions ()

@end

@implementation YTDSuchOptions

- (YTDSuchOptions *)initWithInterfaceField:(YTDTooInternal *)interfaceField
{
    
    self = [super init];

    if (!self)
        return nil;
    
    _interfaceField = interfaceField;

    return self;
}

+ (YTDSuchOptions *)suchOptionsWithInterfaceField:(YTDTooInternal *)interfaceField
{
    return [[YTDSuchOptions alloc] initWithInterfaceField:interfaceField];
}

@end



#pragma mark - YTDVeryOpen

@interface YTDVeryOpen () <YRTSerialization>

@end

#pragma mark - YTDMuchUnprotected

@interface YTDMuchUnprotected () <YRTSerialization>

@end



#pragma mark - YTDWithInternalEnum

@interface YTDWithInternalEnum () <YRTSerialization>

@end
