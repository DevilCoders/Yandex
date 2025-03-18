package {{#NAMESPACE}}{{NAME}}{{#NAMESPACE_separator}}.{{/NAMESPACE_separator}}{{/NAMESPACE}}{{#IS_BINDING}}.internal{{/IS_BINDING}};{{#IMPORT_SECTION}}

{{#IMPORT}}import {{IMPORT_PATH}};{{#IMPORT_separator}}
{{/IMPORT_separator}}{{/IMPORT}}{{/IMPORT_SECTION}}{{#CHILD}}

{{>CHILD}}{{/CHILD}}
