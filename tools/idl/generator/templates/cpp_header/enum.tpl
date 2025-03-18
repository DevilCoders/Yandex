{{#EXCLUDE}}/// @cond EXCLUDE
{{/EXCLUDE}}{{>DOCS}}enum class {{CONSTRUCTOR_NAME}} {{{#ENUM_FIELD}}
{{#EXCLUDE_FIELD}}
    /// @cond EXCLUDE{{/EXCLUDE_FIELD}}
    {{>FIELD_DOCS}}{{FIELD_NAME}}{{#VALUE}} = {{VALUE}}{{/VALUE}}{{#ENUM_FIELD_separator}},{{/ENUM_FIELD_separator}}{{#EXCLUDE_FIELD}}
    /// @endcond{{/EXCLUDE_FIELD}}{{/ENUM_FIELD}}
};{{#EXCLUDE}}
/// @endcond{{/EXCLUDE}}
