{{#IS_NOT_STATIC}}{{#SUBSCRIPTIONS}}namespace {
{{#SUBSCRIPTION}}
char g_{{INSTANCE_NAME:x-cap}}{{LISTENER_INSTANCE_NAME:x-cap}}Key;{{/SUBSCRIPTION}}

} // namespace

@interface {{OBJC_TYPE_NAME}}()
{{#SUBSCRIPTION}}
@property (nonatomic, strong) {{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Subscription *{{LISTENER_INSTANCE_NAME}}Subscription;{{/SUBSCRIPTION}}

@end

{{/SUBSCRIPTIONS}}{{/IS_NOT_STATIC}}@implementation {{OBJC_TYPE_NAME}}{{#IS_CATEGORY}} ({{CATEGORY}}){{/IS_CATEGORY}}{{#IS_NOT_STATIC}}{{#NO_PARENT}} {
    std::{{#STRONG_INTERFACE}}unique{{/STRONG_INTERFACE}}{{#SHARED_INTERFACE}}shared{{/SHARED_INTERFACE}}{{#WEAK_INTERFACE}}weak{{/WEAK_INTERFACE}}_ptr<{{TYPE_NAME}}> _native;
}{{/NO_PARENT}}{{#STRONG_INTERFACE}}

- ({{TYPE_NAME}} *)native{{INSTANCE_NAME:x-cap}}
{
    return {{#HAS_PARENT_INTERFACE}}dynamic_cast<{{TYPE_NAME}}*>([super native{{TOP_MOST_BASE_TYPE_NAME:x-strip-scope}}]){{/HAS_PARENT_INTERFACE}}{{#NO_PARENT}}{{RUNTIME_NAMESPACE_PREFIX}}ios::uniqueGet(_native){{/NO_PARENT}};
}{{/STRONG_INTERFACE}}{{#NOT_STRONG_INTERFACE}}

- (std::shared_ptr<{{TYPE_NAME}}>)native{{INSTANCE_NAME:x-cap}}
{
    return {{#HAS_PARENT_INTERFACE}}std::dynamic_pointer_cast<{{TYPE_NAME}}>([super native{{TOP_MOST_BASE_TYPE_NAME:x-strip-scope}}]){{/HAS_PARENT_INTERFACE}}{{#NO_PARENT}}{{RUNTIME_NAMESPACE_PREFIX}}ios::{{#WEAK_INTERFACE}}weak{{/WEAK_INTERFACE}}{{#SHARED_INTERFACE}}shared{{/SHARED_INTERFACE}}Get(_native){{/NO_PARENT}};
}{{/NOT_STRONG_INTERFACE}}{{#NO_PARENT}}
- ({{#STRONG_INTERFACE}}{{TYPE_NAME}} *{{/STRONG_INTERFACE}}{{#NOT_STRONG_INTERFACE}}std::shared_ptr<{{TYPE_NAME}}>{{/NOT_STRONG_INTERFACE}})native
{
    return [self native{{INSTANCE_NAME:x-cap}}];
}{{/NO_PARENT}}

- (id)init
{
    NSAssert(NO, @"Don't call init on {{OBJC_TYPE_NAME}} - it's a binding implementation! Use factory instead.");
    return nil;
}{{#NO_PARENT}}{{#NOT_WEAK_INTERFACE}}

- (void)deinitialize
{
   _native.reset();
}{{/NOT_WEAK_INTERFACE}}{{/NO_PARENT}}

- (id)initWithWrappedNative:(NSValue *)native
{
    return [self initWithNative:{{#STRONG_INTERFACE}}std::move({{/STRONG_INTERFACE}}*static_cast<std::{{#NOT_STRONG_INTERFACE}}shared{{/NOT_STRONG_INTERFACE}}{{#STRONG_INTERFACE}}unique{{/STRONG_INTERFACE}}_ptr<{{TYPE_NAME}}>*>(native.pointerValue){{#STRONG_INTERFACE}}){{/STRONG_INTERFACE}}];
}

- (id)initWithNative:({{#NOT_STRONG_INTERFACE}}const std::shared_ptr<{{TYPE_NAME}}>&{{/NOT_STRONG_INTERFACE}}{{#STRONG_INTERFACE}}std::unique_ptr<{{TYPE_NAME}}>{{/STRONG_INTERFACE}})native
{
    self = [super init{{#HAS_PARENT_INTERFACE}}WithNative:{{#STRONG_INTERFACE}}std::move({{/STRONG_INTERFACE}}native{{#STRONG_INTERFACE}}){{/STRONG_INTERFACE}}{{/HAS_PARENT_INTERFACE}}];
    if (!self) {
        return nil;
    }
{{#NO_PARENT}}
    NSParameterAssert(native);{{#STRONG_INTERFACE}}
    _native = std::move(native);{{/STRONG_INTERFACE}}{{#NOT_STRONG_INTERFACE}}
    _native = native;{{/NOT_STRONG_INTERFACE}}{{/NO_PARENT}}{{#SUBSCRIPTION}}
    _{{LISTENER_INSTANCE_NAME}}Subscription = [[{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Subscription alloc] initWithKey:&g_{{INSTANCE_NAME:x-cap}}{{LISTENER_INSTANCE_NAME:x-cap}}Key
                                                                          factory:^(id platform{{LISTENER_INSTANCE_NAME:x-cap}})
        {
            auto native{{LISTENER_INSTANCE_NAME:x-cap}} =
                {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toNative<std::shared_ptr<{{LISTENER_TYPE_NAME}}>>(platform{{LISTENER_INSTANCE_NAME:x-cap}});
            return {{RUNTIME_NAMESPACE_PREFIX}}ios::makeSharedObject<{{LISTENER_TYPE_NAME}}>(std::move(native{{LISTENER_INSTANCE_NAME:x-cap}}));
        }
    ];
{{/SUBSCRIPTION}}
    return self;
}{{/IS_NOT_STATIC}}{{#ITEM}}{{#METHOD}}

{{#IS_STATIC}}+{{/IS_STATIC}}{{#IS_NOT_STATIC}}-{{/IS_NOT_STATIC}} ({{OBJC_RESULT_TYPE}}){{OBJC_FUNCTION_NAME}}{{#WITH}}With{{/WITH}}{{#PARAM}}{{OBJC_METHOD_NAME_PARAM_PART}}:({{OBJC_PARAM_TYPE}}){{PARAM_NAME}}{{#PARAM_separator}}
        {{/PARAM_separator}}{{/PARAM}}
{
    BEGIN_BINDING_METHOD;{{#OBJC_NULL_ASSERT}}

    NSParameterAssert({{FIELD_NAME:x-uncap}});{{/OBJC_NULL_ASSERT}}

    {{#RETURNS_SOMETHING}}return {{/RETURNS_SOMETHING}}{{#INIT_ALLOC_RESULT}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform({{/INIT_ALLOC_RESULT}}{{#LISTENER_RESULT}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform({{/LISTENER_RESULT}}{{#SIMPLE_RESULT}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform({{/SIMPLE_RESULT}}{{#STATIC_CAST_RESULT}}static_cast<{{OBJC_RESULT_TYPE}}>({{/STATIC_CAST_RESULT}}{{#IS_STATIC}}::{{#CPP_NAMESPACE}}{{#NAMESPACE}}{{NAME}}::{{/NAMESPACE}}{{/CPP_NAMESPACE}}{{/IS_STATIC}}{{#IS_NOT_STATIC}}self.native{{INSTANCE_NAME:x-cap}}->{{/IS_NOT_STATIC}}{{FUNCTION_NAME}}({{#PARAM}}{{#TO_NATIVE}}
        {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toNative<{{PARAM_TYPE}}>({{PARAM_NAME}}){{/TO_NATIVE}}{{#PARAM_NSERROR}}
        {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toNative<{{PARAM_TYPE}}>({{PARAM_NAME}}){{/PARAM_NSERROR}}{{#PARAM_LAMBDA}}
        {{LISTENER_SCOPE}}ios::{{PARAM_NAME:x-uncap}}([{{LAMBDA_PARAM_NAME}} copy]){{#PARAM_LAMBDA_separator}},{{/PARAM_LAMBDA_separator}}{{/PARAM_LAMBDA}}{{#PARAM_LISTENER}}{{#WEAK_LISTENER_PARAM}}
        {{PARAM_NAME}} == nil ? nullptr : {{RUNTIME_NAMESPACE_PREFIX}}ios::sharedObject_cpp_cast<{{LISTENER_TYPE_NAME}}>(
            [_{{LISTENER_NAME:x-uncap}}Subscription getAssociatedWith:{{PARAM_NAME}}]){{/WEAK_LISTENER_PARAM}}{{#STRONG_LISTENER_PARAM}}
        {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toNative<{{PARAM_TYPE}}>({{PARAM_NAME}}){{/STRONG_LISTENER_PARAM}}{{/PARAM_LISTENER}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}}){{#INIT_ALLOC_RESULT}}){{/INIT_ALLOC_RESULT}}{{#LISTENER_RESULT}}){{/LISTENER_RESULT}}{{#SIMPLE_RESULT}}){{/SIMPLE_RESULT}}{{#STATIC_CAST_RESULT}}){{/STATIC_CAST_RESULT}};

    END_BINDING_METHOD;{{#RETURNS_SOMETHING}}
    return {};{{/RETURNS_SOMETHING}}
}{{/METHOD}}{{#PROPERTY}}

{{#IS_STATIC}}+{{/IS_STATIC}}{{#IS_NOT_STATIC}}-{{/IS_NOT_STATIC}} ({{OBJC_PROPERTY_TYPE}}){{#BOOL}}is{{PROPERTY_NAME:x-cap}}{{/BOOL}}{{#NOT_BOOL}}{{PROPERTY_NAME}}{{/NOT_BOOL}}
{
    BEGIN_BINDING_METHOD;

    return {{#TO_PLATFORM}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform{{/TO_PLATFORM}}{{#LISTENER}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform{{/LISTENER}}{{#STATIC_CAST}}static_cast<{{OBJC_PROPERTY_TYPE}}>{{/STATIC_CAST}}(
        {{#IS_STATIC}}::{{#CPP_NAMESPACE}}{{#NAMESPACE}}{{NAME}}::{{/NAMESPACE}}{{/CPP_NAMESPACE}}{{/IS_STATIC}}{{#IS_NOT_STATIC}}self.native{{INSTANCE_NAME:x-cap}}->{{/IS_NOT_STATIC}}{{PROPERTY_NAME}}());

    END_BINDING_METHOD;
    return {};
}{{#NOT_INTERFACE}}{{#NOT_READONLY}}

{{#IS_STATIC}}+{{/IS_STATIC}}{{#IS_NOT_STATIC}}-{{/IS_NOT_STATIC}} (void)set{{PROPERTY_NAME:x-cap}}:({{OBJC_PROPERTY_TYPE}}){{PROPERTY_NAME}}
{
    BEGIN_BINDING_METHOD;

    {{#OBJC_NULL_ASSERT}}NSParameterAssert({{FIELD_NAME:x-uncap}});

    {{/OBJC_NULL_ASSERT}}{{#IS_STATIC}}::{{#CPP_NAMESPACE}}{{#NAMESPACE}}{{NAME}}::{{/NAMESPACE}}{{/CPP_NAMESPACE}}{{/IS_STATIC}}{{#IS_NOT_STATIC}}self.native{{INSTANCE_NAME:x-cap}}->{{/IS_NOT_STATIC}}set{{PROPERTY_NAME:x-cap}}({{#LISTENER}}{{#WEAK_LISTENER_PROPERTY}}
        {{PROPERTY_NAME}} == nil ? nullptr : {{RUNTIME_NAMESPACE_PREFIX}}ios::sharedObject_cpp_cast<{{PROPERTY_TYPE:x-strip-shared-ptr}}>(
            [_{{PROPERTY_NAME:x-uncap}}Subscription getAssociatedWith:{{PROPERTY_NAME}}]){{/WEAK_LISTENER_PROPERTY}}{{#STRONG_LISTENER_PROPERTY}}
        {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toNative<{{PROPERTY_TYPE}}>({{PROPERTY_NAME}}){{/STRONG_LISTENER_PROPERTY}}{{/LISTENER}}{{#NOT_LISTENER}}
        {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toNative<{{PROPERTY_TYPE}}>({{PROPERTY_NAME}}){{/NOT_LISTENER}});

    END_BINDING_METHOD;
}{{/NOT_READONLY}}{{/NOT_INTERFACE}}{{/PROPERTY}}{{/ITEM}}{{#IS_NOT_STATIC}}{{#NO_PARENT}}{{#WEAK_INTERFACE}}

- (BOOL)isValid
{
    return !_native.expired();
}{{/WEAK_INTERFACE}}{{/NO_PARENT}}{{/IS_NOT_STATIC}}

@end{{#IS_NOT_STATIC}}{{#CHILD}}

{{>CHILD}}{{/CHILD}}
{{#WEAK_INTERFACE}}
{{#NAMESPACE}}namespace {{NAME}} {
{{/NAMESPACE}}
boost::any createPlatform(const std::shared_ptr<{{TYPE_NAME}}>& {{INSTANCE_NAME}})
{
    return (id)[[{{OBJC_TYPE_NAME}} alloc] initWithNative:{{INSTANCE_NAME}}];
}
{{#CLOSING_NAMESPACE}}
} // namespace {{NAME}}{{/CLOSING_NAMESPACE}}{{/WEAK_INTERFACE}}{{/IS_NOT_STATIC}}
