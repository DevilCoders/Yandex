{{CONSTRUCTOR_NAME}}(){{#OPTIONS}} = default{{/OPTIONS}};
{{#EXPLICIT}}explicit {{/EXPLICIT}}{{CONSTRUCTOR_NAME}}({{#PARAM}}
    {{#NOT_POD}}const {{/NOT_POD}}{{TYPE}}{{#NOT_POD}}&{{/NOT_POD}} {{FIELD_NAME}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}});

{{CONSTRUCTOR_NAME}}(const {{CONSTRUCTOR_NAME}}&) = default;
{{CONSTRUCTOR_NAME}}({{CONSTRUCTOR_NAME}}&&) = default;

{{CONSTRUCTOR_NAME}}& operator=(const {{CONSTRUCTOR_NAME}}&) = default;
{{CONSTRUCTOR_NAME}}& operator=({{CONSTRUCTOR_NAME}}&&) = default;