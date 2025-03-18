{{#LITE}}{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace ios {
namespace internal {

template <>
struct ToNative<{{CPP_TYPE_NAME}}, {{OBJC_STRUCT_NAME}}, void> {
    static {{CPP_TYPE_NAME}} from(
        {{OBJC_STRUCT_NAME}}* platform{{CPP_TYPE_NAME:x-strip-scope}});
};

template <typename PlatformType>
struct ToNative<{{CPP_TYPE_NAME}}, PlatformType,
        typename std::enable_if<
            std::is_convertible<PlatformType, {{OBJC_STRUCT_NAME}}*>::value>::type> {
    static {{CPP_TYPE_NAME}} from(
        PlatformType platform{{CPP_TYPE_NAME:x-strip-scope}})
    {
        return ToNative<{{CPP_TYPE_NAME}}, {{OBJC_STRUCT_NAME}}>::from(
            platform{{CPP_TYPE_NAME:x-strip-scope}});
    }
};

template <>
struct ToPlatform<{{CPP_TYPE_NAME}}> {
    static {{OBJC_STRUCT_NAME}}* from(
        const {{CPP_TYPE_NAME}}& {{CPP_TYPE_NAME:x-strip-scope:x-uncap}});
};

} // namespace internal
} // namespace ios
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}{{/LITE}}