{{>DOCS}}public{{#INNER_LEVEL}} static{{/INNER_LEVEL}} class {{TYPE_NAME}} {
{{#TYPES}}
    private {{OPTIONAL_CLASS_TYPE}} {{FIELD_NAME}};{{/TYPES}}{{#TYPES}}

    @NonNull
    public static {{TYPE_NAME}} from{{FIELD_NAME:x-cap}}({{#NONNULL}}@NonNull {{/NONNULL}}{{#NULLABLE}}@Nullable {{/NULLABLE}}{{CLASS_TYPE}} {{FIELD_NAME}}) {
        {{#JAVA_NULL_ASSERT}}if ({{FIELD_NAME}} == null) {
            throw new IllegalArgumentException("Variant value \"{{FIELD_NAME}}\" cannot be null");
        }
        {{/JAVA_NULL_ASSERT}}{{TYPE_NAME}} {{INSTANCE_NAME}} = new {{TYPE_NAME}}();
        {{INSTANCE_NAME}}.{{FIELD_NAME}} = {{FIELD_NAME}};
        return {{INSTANCE_NAME}};
    }{{/TYPES}}{{#TYPES}}

    @Nullable
    public {{OPTIONAL_CLASS_TYPE}} get{{FIELD_NAME:x-cap}}() {
        return {{FIELD_NAME}};
    }{{/TYPES}}
}