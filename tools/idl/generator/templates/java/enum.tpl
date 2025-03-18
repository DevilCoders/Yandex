{{>DOCS}}public{{#INNER_LEVEL}} static{{/INNER_LEVEL}} enum {{TYPE_NAME}} {{{#ENUM_FIELD}}
    {{>FIELD_DOCS}}{{FIELD_NAME}}{{#VALUE}}({{VALUE}}){{/VALUE}}{{#ENUM_FIELD_separator}},{{/ENUM_FIELD_separator}}{{#LAST_ENUM_FIELD}}{{#BITFIELD}};{{/BITFIELD}}{{/LAST_ENUM_FIELD}}{{/ENUM_FIELD}}{{#BITFIELD}}

    public final int value;

    {{TYPE_NAME}}(int value) {
        this.value = value;
    }{{/BITFIELD}}
}