{{#EXCLUDE}}/// @cond EXCLUDE
{{/EXCLUDE}}{{#METHOD}}{{>DOCS}}using {{FUNCTION_NAME}} = std::function<{{RESULT_TYPE}}({{#PARAM}}
    {{#CONST_PARAM}}const {{/CONST_PARAM}}{{PARAM_TYPE}}{{#POINTER_PARAM}}*{{/POINTER_PARAM}}{{#REF_PARAM}}&{{/REF_PARAM}} {{PARAM_NAME}}{{#DEFAULT_VALUE}} ={{VALUE}}{{/DEFAULT_VALUE}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}})>;{{#METHOD_separator}}

{{/METHOD_separator}}{{/METHOD}}{{#EXCLUDE}}
/// @endcond{{/EXCLUDE}}