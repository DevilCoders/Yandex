{{#IS_NOT_STATIC}}@interface {{OBJC_TYPE_NAME}} ()

- (id)initWithWrappedNative:(NSValue *)native;
- (id)initWithNative:({{#STRONG_INTERFACE}}std::unique_ptr<{{TYPE_NAME}}>{{/STRONG_INTERFACE}}{{#NOT_STRONG_INTERFACE}}const std::shared_ptr<{{TYPE_NAME}}>&{{/NOT_STRONG_INTERFACE}})native;{{#STRONG_INTERFACE}}

- ({{TYPE_NAME}} *)native{{INSTANCE_NAME:x-cap}};{{/STRONG_INTERFACE}}{{#NOT_STRONG_INTERFACE}}

- (std::shared_ptr<{{TYPE_NAME}}>)native{{INSTANCE_NAME:x-cap}};{{#NO_PARENT}}
- (std::shared_ptr<{{TYPE_NAME}}>)native;{{/NO_PARENT}}{{/NOT_STRONG_INTERFACE}}

@end{{/IS_NOT_STATIC}}