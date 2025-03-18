{{#NOT_OPTIONS}}{{SCOPE_PREFIX}}{{CONSTRUCTOR_NAME}}::{{CONSTRUCTOR_NAME}}(){{#COLON}}
    : {{/COLON}}{{#PARAM}}{{FIELD_NAME}}(){{#PARAM_separator}},
      {{/PARAM_separator}}{{/PARAM}}
{
}

{{/NOT_OPTIONS}}{{SCOPE_PREFIX}}{{CONSTRUCTOR_NAME}}::{{CONSTRUCTOR_NAME}}(
    {{#PARAM}}{{#NOT_POD}}const {{/NOT_POD}}{{TYPE}}{{#NOT_POD}}&{{/NOT_POD}} {{FIELD_NAME}}{{#PARAM_separator}},
    {{/PARAM_separator}}{{/PARAM}}){{#COLON}}
    : {{/COLON}}{{#PARAM}}{{FIELD_NAME}}{{#OPTIONS}}_{{/OPTIONS}}({{FIELD_NAME}}){{#PARAM_separator}},
      {{/PARAM_separator}}{{/PARAM}}
{
}