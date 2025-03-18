class {{CONSTRUCTOR_NAME}}Binding : public {{TYPE_NAME}} {
public:
    explicit {{CONSTRUCTOR_NAME}}Binding(
        id<{{OBJC_TYPE_NAME}}> platformListener);
{{#METHOD}}
    virtual {{RESULT_TYPE}} {{FUNCTION_NAME}}({{#CPP_PARAM}}{{#PARAM}}
        {{#CONST_PARAM}}const {{/CONST_PARAM}}{{PARAM_TYPE}}{{#REF_PARAM}}&{{/REF_PARAM}} {{PARAM_NAME}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}}{{#CPP_PARAM_separator}},{{/CPP_PARAM_separator}}{{/CPP_PARAM}}){{#CONST_METHOD}} const{{/CONST_METHOD}} override;{{#METHOD_separator}}
{{/METHOD_separator}}{{/METHOD}}

    id<{{OBJC_TYPE_NAME}}> platformReference() const { return platformListener_; }

private:
    {{#WEAK_LISTENER}}__weak {{/WEAK_LISTENER}}id<{{OBJC_TYPE_NAME}}> platformListener_;
};