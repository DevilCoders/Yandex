{{#BRIDGED}}@implementation {{OBJC_STRUCT_NAME}} {
    {{OBJC_RUNTIME_FRAMEWORK_PREFIX}}NativeObject *_nativeObject;{{#FIELD}}{{#VISIBLE_FIELD}}

    {{OBJC_FIELD_TYPE}}{{#POD}} {{/POD}}_{{FIELD_NAME}};
    BOOL _{{FIELD_NAME}}__is_initialized;{{/VISIBLE_FIELD}}{{/FIELD}}
}

- ({{OBJC_STRUCT_NAME}} *)initWithNativeObject:({{OBJC_RUNTIME_FRAMEWORK_PREFIX}}NativeObject*)nativeObject {
    self = [super init];
    if (self) {
        _nativeObject = nativeObject;{{#FIELD}}{{#VISIBLE_FIELD}}
        _{{FIELD_NAME}}__is_initialized = NO;{{/VISIBLE_FIELD}}{{/FIELD}}
    }

    return self;
}

+ ({{OBJC_STRUCT_NAME}} *){{OBJC_INSTANCE_NAME}}With{{#FIELD}}{{#VISIBLE_FIELD}}{{OBJC_METHOD_NAME_FIELD_PART}}:({{OBJC_FIELD_TYPE}}){{FIELD_NAME}}{{/VISIBLE_FIELD}}{{#FIELD_separator}}
                        {{/FIELD_separator}}{{/FIELD}}
{{{#OBJC_NULL_ASSERT}}
    NSParameterAssert({{FIELD_NAME}});{{/OBJC_NULL_ASSERT}}

    BEGIN_BINDING_METHOD;

    auto nativeSelf = std::make_shared<{{TYPE_NAME}}>();
{{#FIELD}}
    nativeSelf->{{FIELD_NAME}} ={{#HIDDEN_FIELD}} {{VALUE}};{{/HIDDEN_FIELD}}
        {{#VISIBLE_FIELD}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toNative<typename std::remove_reference<decltype(nativeSelf->{{FIELD_NAME}})>::type>({{FIELD_NAME}});{{/VISIBLE_FIELD}}{{/FIELD}}

    {{OBJC_STRUCT_NAME}} *obj = [{{OBJC_STRUCT_NAME}} alloc];

    if (obj) {
        obj = [obj initWithNativeObject:
            {{RUNTIME_NAMESPACE_PREFIX}}ios::makeSharedObject<{{TYPE_NAME}}>(nativeSelf)];
{{#FIELD}}{{#VISIBLE_FIELD}}
        obj->_{{FIELD_NAME}} = {{FIELD_NAME}};
        obj->_{{FIELD_NAME}}__is_initialized = YES;{{/VISIBLE_FIELD}}{{/FIELD}}
    }

    return obj;

    END_BINDING_METHOD;
    return {};
}{{#FIELD}}{{#VISIBLE_FIELD}}

- ({{OBJC_FIELD_TYPE}}){{FIELD_NAME}}
{
    @synchronized(self) {
        BEGIN_BINDING_METHOD;

        if (!_{{FIELD_NAME}}__is_initialized) {
            // static_cast here is needed only for enum types.
            _{{FIELD_NAME}} = static_cast<{{OBJC_FIELD_TYPE}}>(
                {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform(
                    {{RUNTIME_NAMESPACE_PREFIX}}ios::sharedObject_cpp_cast<{{TYPE_NAME}}>(_nativeObject)->{{FIELD_NAME}}));
            _{{FIELD_NAME}}__is_initialized = YES;
        }

        END_BINDING_METHOD;
        return _{{FIELD_NAME}};
    }
}{{/VISIBLE_FIELD}}{{/FIELD}}

- ({{OBJC_RUNTIME_FRAMEWORK_PREFIX}}NativeObject*)getNativeObject {
    return _nativeObject;
}

+ (NSString *)getNativeName
{
    return @"{{NATIVE_NAME}}";
}

template boost::any {{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}{{NAME}}::{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}any::internal::BridgedHolder<{{TYPE_NAME}}>::platformObject() const;

@end{{/BRIDGED}}{{#VISIBLE_PLATFORM_OBJECT}}
template boost::any {{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}{{NAME}}::{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}any::internal::BridgedHolder<{{TYPE_NAME}}>::platformObject() const;{{/VISIBLE_PLATFORM_OBJECT}}{{#LITE}}{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace ios {
namespace internal {

{{TYPE_NAME}} ToNative<{{TYPE_NAME}}, {{OBJC_STRUCT_NAME}}, void>::from(
    {{OBJC_STRUCT_NAME}}* platform{{INSTANCE_NAME:x-cap}})
{
    return {{TYPE_NAME}}({{#FIELD}}{{#HIDDEN_FIELD}}
        {{VALUE}}{{/HIDDEN_FIELD}}{{#VISIBLE_FIELD}}
        toNative<{{FIELD_TYPE}}>(platform{{INSTANCE_NAME:x-cap}}.{{FIELD_NAME}}){{/VISIBLE_FIELD}}{{#FIELD_separator}},{{/FIELD_separator}}{{/FIELD}});
}

{{OBJC_STRUCT_NAME}}* ToPlatform<{{TYPE_NAME}}>::from(
    const {{TYPE_NAME}}& native{{INSTANCE_NAME:x-cap}})
{
    return [{{OBJC_STRUCT_NAME}} {{OBJC_INSTANCE_NAME}}With{{#FIELD}}{{#VISIBLE_FIELD}}{{OBJC_METHOD_NAME_FIELD_PART}}:static_cast<{{OBJC_FIELD_TYPE}}>(toPlatform(native{{INSTANCE_NAME:x-cap}}.{{FIELD_NAME}})){{/VISIBLE_FIELD}}{{#FIELD_separator}}
                                 {{/FIELD_separator}}{{/FIELD}}];
}

} // namespace internal
} // namespace ios
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}
{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}{{/LITE}}