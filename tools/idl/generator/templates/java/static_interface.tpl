{{#EXCLUDE}}/** @exclude */
{{/EXCLUDE}}public class {{TYPE_NAME}} {{{#ITEM}}{{#METHOD}}
    {{>METHOD}}{{/METHOD}}{{#PROPERTY}}
    {{>PROPERTY_DOCS}}{{#IS_BINDING}}@Override
    {{/IS_BINDING}}{{#NONNULL}}@NonNull
    {{/NONNULL}}{{#NULLABLE}}@Nullable
    {{/NULLABLE}}public {{#IS_BINDING}}native {{/IS_BINDING}}{{#IS_STATIC}}native static {{/IS_STATIC}}{{PROPERTY_TYPE}} {{#BOOL}}is{{/BOOL}}{{#NOT_BOOL}}get{{/NOT_BOOL}}{{PROPERTY_NAME:x-cap}}();{{#NOT_INTERFACE}}{{#NOT_READONLY}}
    {{#IS_BINDING}}@Override
    {{/IS_BINDING}}public {{#IS_BINDING}}native {{/IS_BINDING}}{{#IS_STATIC}}native static {{/IS_STATIC}}void set{{PROPERTY_NAME:x-cap}}({{#NONNULL}}@NonNull {{/NONNULL}}{{#NULLABLE}}@Nullable {{/NULLABLE}}{{PROPERTY_TYPE}} {{PROPERTY_NAME}});{{/NOT_READONLY}}{{/NOT_INTERFACE}}{{/PROPERTY}}{{#ITEM_separator}}
{{/ITEM_separator}}{{/ITEM}}
}