#import <YandexMapKit/YTDTest.h>
#import <YandexMapsMobile/YRTArchivingUtilities.h>
#import <YandexMapsMobile/YRTExport.h>
#import <YandexMapsMobile/YRTSerialization.h>

#pragma mark - YTDStruct

@interface YTDStruct () <YRTSerialization>

@end

@implementation YTDStruct

- (YTDStruct *)initWithI:(NSInteger)i
{
    
    self = [super init];

    if (!self)
        return nil;
    
    _i = i;

    return self;
}

+ (YTDStruct *)structWithI:(NSInteger)i
{
    return [[YTDStruct alloc] initWithI:i];
}

#pragma mark - YRTSerialization

- (void)serialize:(id<YRTArchiving>)archive
{
    _i = [archive addInteger:_i];
}

@end

@interface YTDVariant () <YRTSerialization>

@end

@implementation YTDVariant

- (YTDVariant *)initWithI:(NSInteger)i
{
   self = [super init];

   if (!self)
       return nil;

   _i = @(i);

   return self;
}

+ (YTDVariant *)variantWithI:(NSInteger)i
{
   return [[YTDVariant alloc] initWithI:i];
}

- (YTDVariant *)initWithF:(float)f
{
   self = [super init];

   if (!self)
       return nil;

   _f = @(f);

   return self;
}

+ (YTDVariant *)variantWithF:(float)f
{
   return [[YTDVariant alloc] initWithF:f];
}

- (void)serialize:(id<YRTArchiving>)archive
{
    NSInteger fieldIndex;
    if (_i) fieldIndex = 0;
    if (_f) fieldIndex = 1;

    fieldIndex = [archive addInteger:fieldIndex];
    
    _i = (fieldIndex != 0) ? nil :
        [archive addInteger:_i
                   optional:NO];
    _f = (fieldIndex != 1) ? nil :
        [archive addFloat:_f
                 optional:NO];
}

@end


#pragma mark - YTDResponse

@interface YTDResponse () <YRTSerialization>

@end

#pragma mark - YTDOptionsStructure

@interface YTDOptionsStructure () <YRTSerialization>

@end

@implementation YTDOptionsStructure

 - (nonnull YTDOptionsStructure *)init {
     self = [super init];

     if (self) {
         self->_filled = "default value";
     }

     return self;
 }

- (YTDOptionsStructure *)initWithEmpty:(NSString *)empty
                                filled:(NSString *)filled
{
    NSParameterAssert(empty);
    NSParameterAssert(filled);
    
    self = [super init];

    if (!self)
        return nil;
    
    _empty = empty;
    _filled = filled;

    return self;
}

+ (YTDOptionsStructure *)optionsStructureWithEmpty:(NSString *)empty
                                            filled:(NSString *)filled
{
    return [[YTDOptionsStructure alloc] initWithEmpty:empty
                                               filled:filled];
}

#pragma mark - YRTSerialization

- (void)serialize:(id<YRTArchiving>)archive
{
    _empty = [archive addString:_empty
                       optional:NO];
    _filled = [archive addString:_filled
                        optional:NO];
}

@end

#pragma mark - YTDDefaultValue

@interface YTDDefaultValue () <YRTSerialization>

@end

@implementation YTDDefaultValue

- (nonnull YTDDefaultValue *)init {
    self = [super init];

    if (self) {
        self->_filled = "default value";
    }

    return self;
}

- (YTDDefaultValue *)initWithFilled:(NSString *)filled
{
    NSParameterAssert(filled);
    
    self = [super init];

    if (!self)
        return nil;
    
    _filled = filled;

    return self;
}

+ (YTDDefaultValue *)defaultValueWithFilled:(NSString *)filled
{
    return [[YTDDefaultValue alloc] initWithFilled:filled];
}

#pragma mark - YRTSerialization

- (void)serialize:(id<YRTArchiving>)archive
{
    _filled = [archive addString:_filled
                        optional:NO];
}

@end

#pragma mark - YTDCombinedValues

@interface YTDCombinedValues () <YRTSerialization>

@end

@implementation YTDCombinedValues

- (nonnull YTDCombinedValues *)init {
    self = [super init];

    if (self) {
        self->_filled = "default value";
    }

    return self;
}

- (YTDCombinedValues *)initWithEmpty:(NSString *)empty
                              filled:(NSString *)filled
{
    NSParameterAssert(empty);
    NSParameterAssert(filled);
    
    self = [super init];

    if (!self)
        return nil;
    
    _empty = empty;
    _filled = filled;

    return self;
}

+ (YTDCombinedValues *)combinedValuesWithEmpty:(NSString *)empty
                                        filled:(NSString *)filled
{
    return [[YTDCombinedValues alloc] initWithEmpty:empty
                                             filled:filled];
}

#pragma mark - YRTSerialization

- (void)serialize:(id<YRTArchiving>)archive
{
    _empty = [archive addString:_empty
                       optional:NO];
    _filled = [archive addString:_filled
                        optional:NO];
}

@end

#pragma mark - YTDDefaultTimeintervalValue

@interface YTDDefaultTimeintervalValue () <YRTSerialization>

@end

@implementation YTDDefaultTimeintervalValue

- (nonnull YTDDefaultTimeintervalValue *)init {
    self = [super init];

    if (self) {
        self->_filled = 0.300000;
    }

    return self;
}

- (YTDDefaultTimeintervalValue *)initWithEmpty:(NSTimeInterval)empty
                                        filled:(NSTimeInterval)filled
{
    
    self = [super init];

    if (!self)
        return nil;
    
    _empty = empty;
    _filled = filled;

    return self;
}

+ (YTDDefaultTimeintervalValue *)defaultTimeintervalValueWithEmpty:(NSTimeInterval)empty
                                                            filled:(NSTimeInterval)filled
{
    return [[YTDDefaultTimeintervalValue alloc] initWithEmpty:empty
                                                       filled:filled];
}

#pragma mark - YRTSerialization

- (void)serialize:(id<YRTArchiving>)archive
{
    _empty = [archive addTimeInterval:_empty];
    _filled = [archive addTimeInterval:_filled];
}

@end




