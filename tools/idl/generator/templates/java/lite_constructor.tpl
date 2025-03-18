public {{CONSTRUCTOR_NAME}}(
        {{#PARAM}}{{#NONNULL}}@NonNull {{/NONNULL}}{{#NULLABLE}}@Nullable {{/NULLABLE}}{{TYPE}} {{FIELD_NAME}}{{#PARAM_separator}},
        {{/PARAM_separator}}{{/PARAM}}) {{{#JAVA_NULL_ASSERT}}
    if ({{FIELD_NAME}} == null) {
        throw new IllegalArgumentException(
            "Required field \"{{FIELD_NAME}}\" cannot be null");
    }{{/JAVA_NULL_ASSERT}}{{#PARAM}}
    this.{{FIELD_NAME}} = {{FIELD_NAME}};{{/PARAM}}
}

/**
 * Use constructor with parameters in your code.
 * This one is for bindings only!
 */
public {{CONSTRUCTOR_NAME}}() {
}