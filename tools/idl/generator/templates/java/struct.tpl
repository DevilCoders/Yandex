{{>DOCS}}public{{#INNER_LEVEL}} static{{/INNER_LEVEL}} class {{CONSTRUCTOR_NAME}} implements Serializable {{{#CHILD}}

    {{>CHILD}}{{/CHILD}}{{#CTORS}}

    {{>CTORS}}{{/CTORS}}{{#LITE}}{{#FIELD}}

    {{#EXCLUDE_FIELD}}/** @exclude */
    {{/EXCLUDE_FIELD}}private {{FIELD_TYPE}} {{FIELD_NAME}}{{#DEFAULT_VALUE}} = {{VALUE}}{{/DEFAULT_VALUE}};

    {{>FIELD_DOCS}}{{#NONNULL}}@NonNull
    {{/NONNULL}}{{#NULLABLE}}@Nullable
    {{/NULLABLE}}public {{FIELD_TYPE}} get{{FIELD_NAME:x-cap}}() {
        return {{FIELD_NAME}};
    }{{/FIELD}}{{/LITE}}{{#BRIDGED}}{{#FIELD}}

    {{#EXCLUDE_FIELD}}/** @exclude */
    {{/EXCLUDE_FIELD}}private {{FIELD_TYPE}} {{FIELD_NAME}};
    {{#EXCLUDE_FIELD}}/** @exclude */
    {{/EXCLUDE_FIELD}}private boolean {{FIELD_NAME}}__is_initialized = false;

    {{>FIELD_DOCS}}{{#NONNULL}}@NonNull
    {{/NONNULL}}{{#NULLABLE}}@Nullable
    {{/NULLABLE}}public synchronized {{FIELD_TYPE}} get{{FIELD_NAME:x-cap}}() {
        if (!{{FIELD_NAME}}__is_initialized) {
            {{FIELD_NAME}} = get{{FIELD_NAME:x-cap}}__Native();
            {{FIELD_NAME}}__is_initialized = true;
        }
        return {{FIELD_NAME}};
    }
    {{#EXCLUDE_FIELD}}/** @exclude */
    {{/EXCLUDE_FIELD}}private native {{FIELD_TYPE}} get{{FIELD_NAME:x-cap}}__Native();{{/FIELD}}{{/BRIDGED}}{{#SERIALIZATION}}

    {{>SERIALIZATION}}{{/SERIALIZATION}}{{#BRIDGED}}

    private NativeObject nativeObject;

    public static String getNativeName() { return "{{NATIVE_NAME}}"; }{{/BRIDGED}}
}