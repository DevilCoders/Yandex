{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace ios {
namespace internal {

{{TYPE_NAME}} ToNative<{{TYPE_NAME}}, id, void>::from(
    id platform{{INSTANCE_NAME:x-cap}})
{
    {{OBJC_VARIANT_NAME}} *typedPlatform{{INSTANCE_NAME:x-cap}} = platform{{INSTANCE_NAME:x-cap}};
{{#TYPES}}
    if (auto platform{{FIELD_NAME:x-cap}} = typedPlatform{{INSTANCE_NAME:x-cap}}.{{FIELD_NAME}}) {
        return {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toNative<{{BIND_CLASS_TYPE}}>(platform{{FIELD_NAME:x-cap}});
    }
{{/TYPES}}
    throw {{RUNTIME_NAMESPACE_PREFIX}}Exception("Invalid variant value");
}

id ToPlatform<{{TYPE_NAME}}>::from(
    const {{TYPE_NAME}}& native{{INSTANCE_NAME:x-cap}})
{
    struct ToObjcVisitor : public boost::static_visitor<id> {{{#TYPES}}
        id operator() (const {{BIND_CLASS_TYPE}}& nativeVariantObject) const
        {
            return [{{OBJC_VARIANT_NAME}} {{INSTANCE_NAME}}With{{FIELD_NAME:x-cap}}: {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform(nativeVariantObject)];
        }{{#TYPES_separator}}
{{/TYPES_separator}}{{/TYPES}}
    };

    return boost::apply_visitor(ToObjcVisitor(), native{{INSTANCE_NAME:x-cap}});
}

} // namespace internal
} // namespace ios
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}