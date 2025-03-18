{{#HEADER}}#pragma once{{/HEADER}}{{#CPP}}#include <{{OWN_HEADER_INCLUDE_PATH}}>{{/CPP}}
{{#IMPORT_SECTION}}
{{#IMPORT}}#include {{IMPORT_PATH}}{{#IMPORT_separator}}
{{/IMPORT_separator}}{{/IMPORT}}{{#IMPORT_SECTION_separator}}
{{/IMPORT_SECTION_separator}}{{/IMPORT_SECTION}}
{{#NAMESPACE}}
namespace {{NAME}} {{{/NAMESPACE}}
{{#FUNCTION}}
{{#DECL}}YANDEX_EXPORT {{/DECL}}{{CPP_TYPE_FULL_NAME}} decode(
    {{#CONST_REF}}const {{/CONST_REF}}{{PB_TYPE_SCOPE}}{{PB_TYPE_NAME}}{{#CONST_REF}}&{{/CONST_REF}} {{#HAS_NO_BODY}}/* {{/HAS_NO_BODY}}proto{{PB_TYPE_NAME}}{{#HAS_NO_BODY}} */{{/HAS_NO_BODY}}){{#DECL}};{{/DECL}}{{#DEF}}
{
    {{>BODY}}
}{{/DEF}}
{{/FUNCTION}}
{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}
{{/CLOSING_NAMESPACE}}