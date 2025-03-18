{{#IS_BINDING}}{{#EXCLUDE}}/** @exclude */
{{/EXCLUDE}}{{/IS_BINDING}}{{#IS_NOT_BINDING}}{{>DOCS}}{{/IS_NOT_BINDING}}public{{#INNER_LEVEL}} static{{/INNER_LEVEL}} {{#IS_BINDING}}class{{/IS_BINDING}}{{#IS_NOT_BINDING}}interface{{/IS_NOT_BINDING}} {{TYPE_NAME}}{{#IS_BINDING}}Binding{{/IS_BINDING}}{{#HAS_PARENT}} extends{{#PARENT}} {{NAME}}{{#IS_BINDING}}Binding{{/IS_BINDING}}{{#PARENT_separator}},{{/PARENT_separator}}{{/PARENT}}{{/HAS_PARENT}}{{#IS_BINDING}} implements {{TYPE_NAME}}{{/IS_BINDING}} {{{#IS_BINDING}}{{#NO_PARENT}}

    /**
     * Holds native interface memory address.
     */
    private final NativeObject nativeObject;{{/NO_PARENT}}

    /**
     * Invoked only from native code.
     */
    protected {{TYPE_NAME}}{{#IS_BINDING}}Binding{{/IS_BINDING}}(NativeObject nativeObject) {
        {{#HAS_PARENT_INTERFACE}}super(nativeObject);{{/HAS_PARENT_INTERFACE}}{{#NO_PARENT}}this.nativeObject = nativeObject;{{/NO_PARENT}}
    }{{/IS_BINDING}}{{#CHILD}}

    {{>CHILD}}{{/CHILD}}{{#ITEM}}{{#METHOD}}

    {{>METHOD}}{{/METHOD}}{{#PROPERTY}}
    {{>PROPERTY_DOCS}}{{#IS_BINDING}}@Override
    {{/IS_BINDING}}{{#NONNULL}}@NonNull
    {{/NONNULL}}{{#NULLABLE}}@Nullable
    {{/NULLABLE}}public {{#IS_BINDING}}native {{/IS_BINDING}}{{PROPERTY_TYPE}} {{#BOOL}}is{{/BOOL}}{{#NOT_BOOL}}get{{/NOT_BOOL}}{{PROPERTY_NAME:x-cap}}();{{#NOT_INTERFACE}}{{#NOT_READONLY}}
    {{#IS_BINDING}}@Override
    {{/IS_BINDING}}public {{#IS_BINDING}}native {{/IS_BINDING}}void set{{PROPERTY_NAME:x-cap}}({{#NONNULL}}@NonNull {{/NONNULL}}{{#NULLABLE}}@Nullable {{/NULLABLE}}{{PROPERTY_TYPE}} {{PROPERTY_NAME}});{{/NOT_READONLY}}{{/NOT_INTERFACE}}{{/PROPERTY}}{{/ITEM}}{{#WEAK_INTERFACE}}{{#NO_PARENT}}

    {{#IS_BINDING}}@Override
    {{/IS_BINDING}}{{#IS_NOT_BINDING}}/**
     * Tells if this {@link {{TYPE_NAME}}} is valid or not. Any other method
     * (except for this one) called on an invalid {@link {{TYPE_NAME}}} will
     * throw {@link java.lang.RuntimeException}. An instance becomes invalid
     * only on UI thread, and only when its implementation depends on objects
     * already destroyed by now. Please refer to general docs about the
     * interface for details on its invalidation.
     */
    {{/IS_NOT_BINDING}}public {{#IS_BINDING}}native {{/IS_BINDING}}boolean isValid();{{/NO_PARENT}}{{/WEAK_INTERFACE}}{{#IS_BINDING}}{{#SUBSCRIPTION}}

    private Subscription<{{LISTENER_NAME}}> {{LISTENER_NAME:x-uncap}}Subscription = new Subscription<{{LISTENER_NAME}}>() {
        @Override
        public NativeObject createNativeListener({{LISTENER_NAME}} listener) {
            return {{TYPE_NAME}}Binding.create{{LISTENER_NAME}}(listener);
        }
    };
    private static native NativeObject create{{LISTENER_NAME}}({{LISTENER_NAME}} listener);{{/SUBSCRIPTION}}{{/IS_BINDING}}
}