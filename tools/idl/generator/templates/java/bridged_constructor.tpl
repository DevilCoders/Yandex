/**
 * Use constructor with parameters in your code.
 * This one is for serialization only!
 */
public {{CONSTRUCTOR_NAME}}() {
}

public {{CONSTRUCTOR_NAME}}(
        {{#PARAM}}{{#NONNULL}}@NonNull {{/NONNULL}}{{#NULLABLE}}@Nullable {{/NULLABLE}}{{TYPE}} {{FIELD_NAME}}{{#PARAM_separator}},
        {{/PARAM_separator}}{{/PARAM}}) {{{#JAVA_NULL_ASSERT}}
    if ({{FIELD_NAME}} == null) {
        throw new IllegalArgumentException(
            "Required field \"{{FIELD_NAME}}\" cannot be null");
    }{{/JAVA_NULL_ASSERT}}
    nativeObject = init({{#PARAM}}
        {{FIELD_NAME}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}});
{{#PARAM}}
    this.{{FIELD_NAME}} = {{FIELD_NAME}};
    this.{{FIELD_NAME}}__is_initialized = true;{{/PARAM}}
}

private native NativeObject init(
        {{#PARAM}}{{TYPE}} {{FIELD_NAME}}{{#PARAM_separator}},
        {{/PARAM_separator}}{{/PARAM}});

private {{CONSTRUCTOR_NAME}}(NativeObject nativeObject) {
    this.nativeObject = nativeObject;
}