{{>DOCS}}class YANDEX_EXPORT {{CONSTRUCTOR_NAME}} {
public:{{#CHILD}}

    {{>CHILD}}{{/CHILD}}{{#CTORS}}

    {{>CTORS}}{{/CTORS}}{{#FIELD}}

    {{#NOT_POD}}const {{/NOT_POD}}{{FIELD_TYPE}}{{#NOT_POD}}&{{/NOT_POD}} {{FIELD_NAME}}() const { return {{FIELD_NAME}}_; }
    {{CONSTRUCTOR_NAME}}& set{{FIELD_NAME:x-cap}}({{#NOT_POD}}const {{/NOT_POD}}{{FIELD_TYPE}}{{#NOT_POD}}&{{/NOT_POD}} {{FIELD_NAME}})
    {
        {{FIELD_NAME}}_ = {{FIELD_NAME}};
        return *this;
    }{{/FIELD}}{{#SERIAL}}

    {{>SERIAL}}{{/SERIAL}}

private:{{#FIELD}}
    {{>FIELD_DOCS}}{{FIELD_TYPE}} {{FIELD_NAME}}_{{#DEFAULT_VALUE}}{ {{VALUE}} }{{/DEFAULT_VALUE}};{{#FIELD_separator}}
    {{/FIELD_separator}}{{/FIELD}}
};