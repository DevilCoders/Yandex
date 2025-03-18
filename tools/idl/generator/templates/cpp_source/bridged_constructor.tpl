{{SCOPE_PREFIX}}{{CONSTRUCTOR_NAME}}::{{CONSTRUCTOR_NAME}}(){{#COLON}}
    : {{/COLON}}{{#PARAM}}{{FIELD_NAME}}({{#BRIDGED_FIELD}}{{#NONOPTIONAL_FIELD}}std::make_shared<{{TYPE:x-strip-shared-ptr}}>(){{/NONOPTIONAL_FIELD}}{{/BRIDGED_FIELD}}){{#PARAM_separator}},
      {{/PARAM_separator}}{{/PARAM}}
{
}

{{SCOPE_PREFIX}}{{CONSTRUCTOR_NAME}}::{{CONSTRUCTOR_NAME}}({{#PARAM}}
    {{#NOT_POD}}const {{/NOT_POD}}{{#LITE_FIELD}}{{TYPE}}{{/LITE_FIELD}}{{#BRIDGED_FIELD}}{{#OPTIONAL_FIELD}}{{#STD_OPTIONAL}}std::optional{{/STD_OPTIONAL}}{{#BOOST_OPTIONAL}}boost::optional{{/BOOST_OPTIONAL}}<{{/OPTIONAL_FIELD}}{{TYPE:x-strip-shared-ptr}}{{#OPTIONAL_FIELD}}>{{/OPTIONAL_FIELD}}{{/BRIDGED_FIELD}}{{#NOT_POD}}&{{/NOT_POD}} {{FIELD_NAME}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}}){{#COLON}}
    : {{/COLON}}{{#PARAM}}{{FIELD_NAME}}({{#BRIDGED_FIELD}}{{#OPTIONAL_FIELD}}{{FIELD_NAME}} ? {{/OPTIONAL_FIELD}}std::make_shared<{{TYPE:x-strip-shared-ptr}}>({{#OPTIONAL_FIELD}}*{{/OPTIONAL_FIELD}}{{/BRIDGED_FIELD}}{{FIELD_NAME}}{{#BRIDGED_FIELD}}){{#OPTIONAL_FIELD}} : nullptr{{/OPTIONAL_FIELD}}{{/BRIDGED_FIELD}}){{#PARAM_separator}},
      {{/PARAM_separator}}{{/PARAM}}
{
}{{#HAS_BRIDGED_FIELDS}}

{{SCOPE_PREFIX}}{{CONSTRUCTOR_NAME}}::{{CONSTRUCTOR_NAME}}(const {{CONSTRUCTOR_NAME}}& other){{#COLON}}
    : {{/COLON}}{{#PARAM}}{{FIELD_NAME}}({{#BRIDGED_FIELD}}{{#OPTIONAL_FIELD}}other.{{FIELD_NAME}} ? {{/OPTIONAL_FIELD}}std::make_shared<{{TYPE:x-strip-shared-ptr}}>(*{{/BRIDGED_FIELD}}other.{{FIELD_NAME}}{{#BRIDGED_FIELD}}){{#OPTIONAL_FIELD}} : nullptr{{/OPTIONAL_FIELD}}{{/BRIDGED_FIELD}}){{#PARAM_separator}},
      {{/PARAM_separator}}{{/PARAM}}
{
}

{{SCOPE_PREFIX}}{{CONSTRUCTOR_NAME}}& {{SCOPE_PREFIX}}{{CONSTRUCTOR_NAME}}::operator=(const {{CONSTRUCTOR_NAME}}& other)
{{{#PARAM}}{{#LITE_FIELD}}
    {{FIELD_NAME}} = other.{{FIELD_NAME}};{{/LITE_FIELD}}{{#BRIDGED_FIELD}}
    {{FIELD_NAME}} = {{#OPTIONAL_FIELD}}other.{{FIELD_NAME}} ? {{/OPTIONAL_FIELD}}std::make_shared<{{TYPE:x-strip-shared-ptr}}>(*other.{{FIELD_NAME}}){{#OPTIONAL_FIELD}} : nullptr{{/OPTIONAL_FIELD}};{{/BRIDGED_FIELD}}{{/PARAM}}
    return *this;
}{{/HAS_BRIDGED_FIELDS}}