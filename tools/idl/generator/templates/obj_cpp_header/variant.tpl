{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace ios {
namespace internal {

template <>
struct ToNative<{{TYPE_NAME}}, id, void> {
    static {{TYPE_NAME}} from(
        id platform{{INSTANCE_NAME:x-cap}});
};

template <typename PlatformType>
struct ToNative<{{TYPE_NAME}}, PlatformType,
        typename std::enable_if<
            std::is_convertible<PlatformType, id>::value>::type> {
    static {{TYPE_NAME}} from(
        PlatformType platform{{INSTANCE_NAME:x-cap}})
    {
        return ToNative<{{TYPE_NAME}}, id>::from(
            platform{{INSTANCE_NAME:x-cap}});
    }
};

template <>
struct ToPlatform<{{TYPE_NAME}}> {
    static id from(
        const {{TYPE_NAME}}& {{INSTANCE_NAME}});
};

} // namespace internal
} // namespace ios
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}