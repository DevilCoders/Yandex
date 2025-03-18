{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace ios {
namespace internal {

{{TYPE_NAME}} ToNative<{{TYPE_NAME}}, {{OBJC_STRUCT_NAME}}, void>::from(
    {{OBJC_STRUCT_NAME}}* platform{{INSTANCE_NAME:x-cap}})
{
    return {{TYPE_NAME}}({{#FIELD}}{{#HIDDEN_FIELD}}
        {{VALUE}}{{/HIDDEN_FIELD}}{{#VISIBLE_FIELD}}
        toNative<{{FIELD_TYPE}}>(platform{{INSTANCE_NAME:x-cap}}.{{FIELD_NAME}}){{/VISIBLE_FIELD}}{{#FIELD_separator}},{{/FIELD_separator}}{{/FIELD}});
}

{{OBJC_STRUCT_NAME}}* ToPlatform<{{TYPE_NAME}}>::from(
    const {{TYPE_NAME}}& native{{INSTANCE_NAME:x-cap}})
{
    return [{{OBJC_STRUCT_NAME}} {{OBJC_INSTANCE_NAME}}With{{#FIELD}}{{#VISIBLE_FIELD}}{{OBJC_METHOD_NAME_FIELD_PART}}:static_cast<{{OBJC_FIELD_TYPE}}>(toPlatform(native{{INSTANCE_NAME:x-cap}}.{{FIELD_NAME}}())){{/VISIBLE_FIELD}}{{#FIELD_separator}}
                                 {{/FIELD_separator}}{{/FIELD}}];
}

} // namespace internal
} // namespace ios
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}
