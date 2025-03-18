{{>DOCS}}typedef void(^{{TYPE_NAME}})(
    {{#METHOD}}{{#PARAM}}{{PARAM_TYPE}}{{#OPTIONAL}} _Nullable {{/OPTIONAL}}{{#NON_OPTIONAL}}{{#MERGED}} _Nullable {{/MERGED}}{{#NON_MERGED}} _Nonnull {{/NON_MERGED}}{{/NON_OPTIONAL}}{{#POD}} {{/POD}}{{PARAM_NAME}}{{#PARAM_separator}},
    {{/PARAM_separator}}{{/PARAM}}{{#METHOD_separator}},
    {{/METHOD_separator}}{{/METHOD}}{{#VOID_EMPTY}}void{{/VOID_EMPTY}});