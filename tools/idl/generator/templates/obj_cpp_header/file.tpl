{{#IMPORT_SECTION}}{{#IMPORT}}#import {{IMPORT_PATH}}{{#IMPORT_separator}}
{{/IMPORT_separator}}{{/IMPORT}}{{#IMPORT_SECTION_separator}}

{{/IMPORT_SECTION_separator}}{{/IMPORT_SECTION}}{{#NAMESPACE_SECTION}}
{{#NAMESPACE}}
namespace {{NAME}} {{{/NAMESPACE}}{{/NAMESPACE_SECTION}}{{#CHILD}}

{{>CHILD}}{{/CHILD}}{{#NAMESPACE_SECTION}}
{{#CLOSING_NAMESPACE}}
} // namespace {{NAME}}{{/CLOSING_NAMESPACE}}{{/NAMESPACE_SECTION}}{{#TOPLEVEL_CHILD}}

{{>TOPLEVEL_CHILD}}{{/TOPLEVEL_CHILD}}{{#LISTENER_TO_NATIVE_TO_PLATFORM}}

{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace ios {
namespace internal {

template <>
struct ToNative<std::shared_ptr<{{CPP_TYPE_NAME}}>, {{OBJC_TYPE_NAME}}, void> {
    static std::shared_ptr<{{CPP_TYPE_NAME}}> from(
        {{OBJC_TYPE_NAME}} platform{{INSTANCE_NAME:x-cap}});
};
template <typename PlatformType>
struct ToNative<std::shared_ptr<{{CPP_TYPE_NAME}}>, PlatformType> {
    static std::shared_ptr<{{CPP_TYPE_NAME}}> from(
        PlatformType platform{{INSTANCE_NAME:x-cap}})
    {
        return ToNative<std::shared_ptr<{{CPP_TYPE_NAME}}>, {{OBJC_TYPE_NAME}}>::from(
            platform{{INSTANCE_NAME:x-cap}});
    }
};

template <>
struct ToPlatform<std::shared_ptr<{{CPP_TYPE_NAME}}>> {
    static {{OBJC_TYPE_NAME}} from(
        const std::shared_ptr<{{CPP_TYPE_NAME}}>& native{{INSTANCE_NAME:x-cap}});
};

} // namespace internal
} // namespace ios
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}{{/LISTENER_TO_NATIVE_TO_PLATFORM}}
