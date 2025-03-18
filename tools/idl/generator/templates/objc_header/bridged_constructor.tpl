+ ({{#OPTIONAL_RESULT}}nullable {{/OPTIONAL_RESULT}}{{#NON_OPTIONAL_RESULT}}nonnull {{/NON_OPTIONAL_RESULT}}{{TYPE_NAME}} *){{INSTANCE_NAME}}With{{#PARAM}}{{OBJC_METHOD_NAME_FIELD_PART}}:({{#OPTIONAL}}nullable{{/OPTIONAL}}{{#NON_OPTIONAL}}nonnull{{/NON_OPTIONAL}} {{TYPE}}){{FIELD_NAME}}{{#PARAM_separator}}
                        {{/PARAM_separator}}{{/PARAM}};
