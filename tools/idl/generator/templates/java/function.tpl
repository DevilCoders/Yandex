{{#IS_NOT_HEADER}}{{#EXCLUDE}}/** @exclude */
{{/EXCLUDE}}{{/IS_NOT_HEADER}}{{#IS_HEADER}}{{>DOCS}}{{#UI_THREAD}}@UiThread
{{/UI_THREAD}}{{#BG_THREAD}}@WorkerThread
{{/BG_THREAD}}{{#ANY_THREAD}}@AnyThread
{{/ANY_THREAD}}{{/IS_HEADER}}{{#IS_NOT_HEADER}}@Override
{{/IS_NOT_HEADER}}{{#RESULT_NONNULL}}@NonNull
{{/RESULT_NONNULL}}{{#RESULT_NULLABLE}}@Nullable
{{/RESULT_NULLABLE}}public {{#IS_NOT_HEADER}}native {{/IS_NOT_HEADER}}{{#IS_STATIC}}native static {{/IS_STATIC}}{{RESULT_TYPE}} {{FUNCTION_NAME}}({{#PARAM}}
    {{#NONNULL}}@NonNull {{/NONNULL}}{{#NULLABLE}}@Nullable {{/NULLABLE}}{{PARAM_TYPE}} {{PARAM_NAME}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}});