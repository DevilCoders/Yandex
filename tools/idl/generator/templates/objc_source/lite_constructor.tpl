- ({{TYPE_NAME}} *)initWith{{#PARAM}}{{OBJC_METHOD_NAME_FIELD_PART}}:({{TYPE}}){{FIELD_NAME}}{{#PARAM_separator}}
                        {{/PARAM_separator}}{{/PARAM}}
{
    {{#OBJC_NULL_ASSERT}}NSParameterAssert({{FIELD_NAME}});
    {{/OBJC_NULL_ASSERT}}
    self = [super init];

    if (!self)
        return nil;
    {{#PARAM}}
    _{{FIELD_NAME}} = {{FIELD_NAME}};{{/PARAM}}

    return self;
}

+ ({{TYPE_NAME}} *){{INSTANCE_NAME}}With{{#PARAM}}{{OBJC_METHOD_NAME_FIELD_PART}}:({{TYPE}}){{FIELD_NAME}}{{#PARAM_separator}}
                        {{/PARAM_separator}}{{/PARAM}}
{
    return [[{{TYPE_NAME}} alloc] initWith{{#PARAM}}{{OBJC_METHOD_NAME_FIELD_PART}}:{{FIELD_NAME}}{{#PARAM_separator}}
                        {{/PARAM_separator}}{{/PARAM}}];
}