{{>DOCS}}typedef NS_{{#SIMPLE_ENUM}}ENUM{{/SIMPLE_ENUM}}{{#BITFIELD}}OPTIONS{{/BITFIELD}}(NSUInteger, {{TYPE_NAME}}) {{{#ENUM_FIELD}}
    {{>FIELD_DOCS}}{{FIELD_NAME}}{{#VALUE}} = {{VALUE}}{{/VALUE}}{{#ENUM_FIELD_separator}},{{/ENUM_FIELD_separator}}{{/ENUM_FIELD}}
};