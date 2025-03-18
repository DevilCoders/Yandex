#pragma mark - {{TYPE_NAME}}

@interface {{TYPE_NAME}} () <{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Serialization>

@end{{#LITE}}

@implementation {{TYPE_NAME}}{{#HAVE_DEFAULT_VALUE}}

- ({{#OPTIONAL_RESULT}}nullable{{/OPTIONAL_RESULT}}{{#NON_OPTIONAL_RESULT}}nonnull{{/NON_OPTIONAL_RESULT}} {{TYPE_NAME}} *)init {
    self = [super init];

    if (self) {{{#FIELD}}{{#DEFAULT_VALUE}}
        self->_{{FIELD_NAME}} = {{VALUE}};{{/DEFAULT_VALUE}}{{/FIELD}}
    }

    return self;
}{{/HAVE_DEFAULT_VALUE}}{{#CTORS}}

{{>CTORS}}{{/CTORS}}{{#SERIALIZATION}}

{{>SERIALIZATION}}{{/SERIALIZATION}}

@end{{/LITE}}