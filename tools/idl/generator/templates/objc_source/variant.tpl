@interface {{TYPE_NAME}} () <{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Serialization>

@end

@implementation {{TYPE_NAME}}
{{#TYPES}}
- ({{TYPE_NAME}} *)initWith{{FIELD_NAME:x-cap}}:({{CLASS_TYPE}}){{FIELD_NAME}}
{
   {{#OBJC_NULL_ASSERT}}NSParameterAssert({{FIELD_NAME}});

   {{/OBJC_NULL_ASSERT}}self = [super init];

   if (!self)
       return nil;

   _{{FIELD_NAME}} = {{#POD}}@({{/POD}}{{FIELD_NAME}}{{#POD}}){{/POD}};

   return self;
}

+ ({{TYPE_NAME}} *){{INSTANCE_NAME}}With{{FIELD_NAME:x-cap}}:({{CLASS_TYPE}}){{FIELD_NAME}}
{
   return [[{{TYPE_NAME}} alloc] initWith{{FIELD_NAME:x-cap}}:{{FIELD_NAME}}];
}
{{/TYPES}}
- (void)serialize:(id<{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Archiving>)archive
{
    NSInteger fieldIndex;{{#TYPES}}
    if (_{{FIELD_NAME}}) fieldIndex = {{FIELD_INDEX}};{{/TYPES}}

    fieldIndex = [archive addInteger:fieldIndex];
    {{#TYPES}}
    _{{FIELD_NAME}} = (fieldIndex != {{FIELD_INDEX}}) ? nil :
        [archive add{{TYPE}}:_{{FIELD_NAME}}
                    optional:NO{{#CUSTOM}}
                       class:[{{FIELD_CLASS}} class]{{/CUSTOM}}{{#ARRAY}}
                       {{HANDLER}}:{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Archiving{{HANDLER_TYPE}}Handler({{#FOR_VARIANT}}@[{{/FOR_VARIANT}}{{#CUSTOM_HANDLER}}[{{FIELD_CLASS}} class]{{#CUSTOM_HANDLER_separator}}
                       {{/CUSTOM_HANDLER_separator}}{{/CUSTOM_HANDLER}}{{#FOR_VARIANT}}]{{/FOR_VARIANT}}){{/ARRAY}}{{#DICTIONARY}}
                    keyHandler:{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Archiving{{HANDLER_TYPE}}HandlerkeyHandler
                  valueHandler:{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Archiving{{HANDLER_TYPE}}HandlervalueHandler];
                {{/DICTIONARY}}{{#VARIANT}}
                    classArray:@[{{#CLASS}}[{{FIELD_CLASS}} class]{{#CLASS_separator}},
                                    {{/CLASS_separator}}{{/CLASS}}];
                    {{/VARIANT}}];{{/TYPES}}
}

@end
