{{#IMPORT_SECTION}}{{#IMPORT}}#import {{IMPORT_PATH}}{{#IMPORT_separator}}
{{/IMPORT_separator}}{{/IMPORT}}{{#IMPORT_SECTION_separator}}

{{/IMPORT_SECTION_separator}}{{/IMPORT_SECTION}}{{#NAMESPACE_SECTION}}
{{#NAMESPACE}}
namespace {{NAME}} {{{/NAMESPACE}}{{/NAMESPACE_SECTION}}{{#CHILD}}

{{>CHILD}}{{/CHILD}}{{#NAMESPACE_SECTION}}
{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/NAMESPACE_SECTION}}{{#TOPLEVEL_CHILD}}

{{>TOPLEVEL_CHILD}}{{/TOPLEVEL_CHILD}}{{#LISTENER_TO_NATIVE_TO_PLATFORM}}

{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace ios {
namespace internal {

std::shared_ptr<{{CPP_TYPE_NAME}}> ToNative<std::shared_ptr<{{CPP_TYPE_NAME}}>, {{OBJC_TYPE_NAME}}, void>::from(
    {{OBJC_TYPE_NAME}} platform{{INSTANCE_NAME:x-cap}})
{
    if (!platform{{INSTANCE_NAME:x-cap}}) {
        return { };
    }

    return std::make_shared<{{BINDING_TYPE_NAME}}>(
        platform{{INSTANCE_NAME:x-cap}});
}

{{OBJC_TYPE_NAME}} ToPlatform<std::shared_ptr<{{CPP_TYPE_NAME}}>>::from(
    const std::shared_ptr<{{CPP_TYPE_NAME}}>& native{{INSTANCE_NAME:x-cap}})
{
    if (!native{{INSTANCE_NAME:x-cap}}) {
        return nil;
    }

    return static_cast<{{BINDING_TYPE_NAME}}*>(native{{INSTANCE_NAME:x-cap}}.get())->platformReference();
}

} // namespace internal
} // namespace ios
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}{{/LISTENER_TO_NATIVE_TO_PLATFORM}}
