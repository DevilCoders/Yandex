{{>DOCS}}public{{#INNER_LEVEL}} static{{/INNER_LEVEL}} final class {{CONSTRUCTOR_NAME}}{{#SERIALIZATION}} implements Serializable{{/SERIALIZATION}} {{{#CHILD}}

    {{>CHILD}}{{/CHILD}}{{#CTORS}}

    {{>CTORS}}{{/CTORS}}{{#FIELD}}

    {{#EXCLUDE_FIELD}}/** @exclude */
    {{/EXCLUDE_FIELD}}private {{FIELD_TYPE}} {{FIELD_NAME}}{{#DEFAULT_VALUE}} = {{VALUE}}{{/DEFAULT_VALUE}};

    {{>FIELD_DOCS}}{{#NONNULL}}@NonNull
    {{/NONNULL}}{{#NULLABLE}}@Nullable
    {{/NULLABLE}}public {{FIELD_TYPE}} get{{FIELD_NAME:x-cap}}() {
        return {{FIELD_NAME}};
    }

    /**{{#EXCLUDE_FIELD}} @exclude{{/EXCLUDE_FIELD}}
     * See {@link #get{{FIELD_NAME:x-cap}}()}.
     */
    public {{CONSTRUCTOR_NAME}} set{{FIELD_NAME:x-cap}}({{#NONNULL}}@NonNull {{/NONNULL}}{{#NULLABLE}}@Nullable {{/NULLABLE}}{{FIELD_TYPE}} {{FIELD_NAME}}) {
        {{#JAVA_NULL_ASSERT}}if ({{FIELD_NAME}} == null) {
            throw new IllegalArgumentException(
                "Required field \"{{FIELD_NAME}}\" cannot be null");
        }
        {{/JAVA_NULL_ASSERT}}this.{{FIELD_NAME}} = {{FIELD_NAME}};
        return this;
    }{{/FIELD}}{{#SERIALIZATION}}

    {{>SERIALIZATION}}{{/SERIALIZATION}}
}