{{#EXCLUDE}}/// @cond EXCLUDE
{{/EXCLUDE}}{{>DOCS}}struct YANDEX_EXPORT {{CONSTRUCTOR_NAME}} {{{#CHILD}}

    {{>CHILD}}{{/CHILD}}{{#CTORS}}

    {{>CTORS}}{{/CTORS}}{{#FIELD}}

    {{>FIELD_DOCS}}{{FIELD_TYPE}} {{FIELD_NAME}}{{#DEFAULT_VALUE}}{ {{VALUE}} }{{/DEFAULT_VALUE}};{{/FIELD}}
};{{#EXCLUDE}}
/// @endcond{{/EXCLUDE}}