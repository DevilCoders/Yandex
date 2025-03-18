{{#EMPTY_LINE}}
{{/EMPTY_LINE}}{{>DOCS}}using {{CONSTRUCTOR_NAME}} = boost::variant<
    {{#TYPES}}{{CLASS_TYPE}}{{#TYPES_separator}},
    {{/TYPES_separator}}{{/TYPES}}>;