#pragma mark - {{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Serialization

- (void)serialize:(id<{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Archiving>)archive
{{{#BRIDGED}}
    [NSException
        raise:@"Not implemeted"
        format:@"Serialization for bridged objects is not implemented"];{{/BRIDGED}}{{#LITE}}{{#FIELD}}
    _{{FIELD_NAME}} = {{#ENUM_CAST}}({{ENUM_TYPE}}){{/ENUM_CAST}}[archive add{{TYPE}}:_{{FIELD_NAME}}{{#OPTIONAL}}
                          optional:{{OPTIONAL_VALUE}}{{/OPTIONAL}}{{#CUSTOM}}
                             class:[{{FIELD_CLASS}} class]{{/CUSTOM}}{{#ARRAY}}
                           {{HANDLER}}:{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Archiving{{HANDLER_TYPE}}Handler({{#FOR_VARIANT}}@[{{/FOR_VARIANT}}{{#CUSTOM_HANDLER}}[{{FIELD_CLASS}} class]{{#CUSTOM_HANDLER_separator}},{{/CUSTOM_HANDLER_separator}}{{/CUSTOM_HANDLER}}{{#FOR_VARIANT}}]{{/FOR_VARIANT}}){{/ARRAY}}{{#DICTIONARY}}
                        keyHandler:{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Archiving{{HANDLER_TYPE}}HandlerkeyHandler
                      valueHandler:{{OBJC_RUNTIME_FRAMEWORK_PREFIX}}Archiving{{HANDLER_TYPE}}HandlervalueHandler];
                    {{/DICTIONARY}}{{#VARIANT}}
                        classArray:@[{{#CLASS}}
                                [{{FIELD_CLASS}} class]{{#CLASS_separator}},{{/CLASS_separator}}{{/CLASS}}]{{/VARIANT}}];{{/FIELD}}{{/LITE}}
}