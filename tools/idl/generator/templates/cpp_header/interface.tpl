{{#EXCLUDE}}/// @cond EXCLUDE
{{/EXCLUDE}}{{>DOCS}}class YANDEX_EXPORT {{CONSTRUCTOR_NAME}}{{#HAS_PARENT}} :{{#PARENT}} public{{#VIRTUAL}} virtual{{/VIRTUAL}} {{NAME}}{{#PARENT_separator}},{{/PARENT_separator}}{{/PARENT}}{{/HAS_PARENT}} {
public:
    virtual ~{{CONSTRUCTOR_NAME}}()
    {
    }{{#CHILD}}

    {{>CHILD}}{{/CHILD}}{{#ITEM}}{{#METHOD}}

    {{>METHOD}}{{/METHOD}}{{#PROPERTY}}
{{#PROPERTY_EXCLUDE}}
    /// @cond EXCLUDE{{/PROPERTY_EXCLUDE}}
    {{>PROPERTY_DOCS}}{{#INTERFACE}}virtual {{#NOT_CONST}}const {{/NOT_CONST}}{{PROPERTY_TYPE}} {{PROPERTY_NAME}}() const = 0;{{#NOT_CONST}}
    virtual {{PROPERTY_TYPE}} {{PROPERTY_NAME}}() = 0;{{/NOT_CONST}}{{/INTERFACE}}{{#NOT_INTERFACE}}virtual {{#NOT_LISTENER}}{{#NOT_GEN}}{{#NOT_POD}}const {{/NOT_POD}}{{/NOT_GEN}}{{/NOT_LISTENER}}{{PROPERTY_TYPE}}{{#NOT_GEN}}{{#NOT_POD}}&{{/NOT_POD}}{{/NOT_GEN}} {{PROPERTY_NAME}}() const = 0;{{#NOT_READONLY}}
    virtual void set{{PROPERTY_NAME:x-cap}}({{#NOT_LISTENER}}{{#NOT_POD}}const {{/NOT_POD}}{{/NOT_LISTENER}}{{PROPERTY_TYPE}}{{#NOT_POD}}&{{/NOT_POD}} {{PROPERTY_NAME}}) = 0;{{/NOT_READONLY}}{{/NOT_INTERFACE}}{{#PROPERTY_EXCLUDE}}
    /// @endcond{{/PROPERTY_EXCLUDE}}{{/PROPERTY}}{{/ITEM}}
};{{#EXCLUDE}}
/// @endcond{{/EXCLUDE}}