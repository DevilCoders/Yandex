#pragma once

{{#IMPORT_SECTION}}{{#IMPORT}}#include {{IMPORT_PATH}}
{{/IMPORT}}
{{/IMPORT_SECTION}}{{#CLASSIC_LISTENER}}{{#NAMESPACE}}namespace {{NAME}} {
{{/NAMESPACE}}namespace android {

class YANDEX_EXPORT {{CONSTRUCTOR_NAME}}Binding : public {{TYPE_NAME}} {
public:
    explicit {{CONSTRUCTOR_NAME}}Binding(
        {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject {{INSTANCE_NAME}});{{#METHOD}}

    virtual {{RESULT_TYPE}} {{FUNCTION_NAME}}({{#PARAMETER}}{{#PARAM}}
        {{#CONST_PARAM}}const {{/CONST_PARAM}}{{PARAM_TYPE}}{{#REF_PARAM}}&{{/REF_PARAM}} {{PARAM_NAME}}{{/PARAM}}{{#PARAMETER_separator}},{{/PARAMETER_separator}}{{/PARAMETER}}){{#CONST_METHOD}} const{{/CONST_METHOD}} override;{{/METHOD}}

    {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject platformReference() const
    {
        return {{INSTANCE_NAME}}_{{#WEAK_LISTENER}}.lock(){{/WEAK_LISTENER}};
    }

private:
    static const std::string JNI_TYPE_REF;

    {{RUNTIME_NAMESPACE_PREFIX}}android::Jni{{#STRONG_LISTENER}}Object{{/STRONG_LISTENER}}{{#WEAK_LISTENER}}Weak{{/WEAK_LISTENER}} {{INSTANCE_NAME}}_;
};

} // namespace android{{#CLOSING_NAMESPACE}}
} // namespace {{NAME}}{{/CLOSING_NAMESPACE}}

{{/CLASSIC_LISTENER}}{{#LAMBDA_LISTENER}}{{#NAMESPACE}}namespace {{NAME}} {
{{/NAMESPACE}}namespace android {{{#METHOD}}

YANDEX_EXPORT {{SCOPE_PREFIX}}{{FUNCTION_NAME}} create{{FUNCTION_NAME}}(
    {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject {{INSTANCE_NAME}});{{/METHOD}}

} // namespace android{{#CLOSING_NAMESPACE}}
} // namespace {{NAME}}{{/CLOSING_NAMESPACE}}

{{/LAMBDA_LISTENER}}{{#VARIANT}}{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace android {
namespace internal {

template <>
struct ToNative<{{TYPE_NAME}}, jobject, void> {
    static {{TYPE_NAME}} from(
        jobject platform{{INSTANCE_NAME:x-cap}});
};
template <>
struct ToNative<{{TYPE_NAME}}, {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject, void> {
    static {{TYPE_NAME}} from(
        {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject platform{{INSTANCE_NAME:x-cap}})
    {
        return ToNative<{{TYPE_NAME}}, jobject>::from(
            platform{{INSTANCE_NAME:x-cap}}.get());
    }
};

template <>
struct ToPlatform<{{TYPE_NAME}}> {
    static {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject from(
        const {{TYPE_NAME}}& {{INSTANCE_NAME}});
};

} // namespace internal
} // namespace android
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}

{{/VARIANT}}{{#STRUCT}}{{#LITE}}{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace android {
namespace internal {

template <>
struct ToNative<{{TYPE_NAME}}, jobject, void> {
    static {{TYPE_NAME}} from(
        jobject platform{{INSTANCE_NAME:x-cap}});
};
template <>
struct ToNative<{{TYPE_NAME}}, {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject> {
    static {{TYPE_NAME}} from(
        {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject platform{{INSTANCE_NAME:x-cap}})
    {
        return ToNative<{{TYPE_NAME}}, jobject>::from(
            platform{{INSTANCE_NAME:x-cap}}.get());
    }
};

template <>
struct ToPlatform<{{TYPE_NAME}}> {
    static JniObject from(
        const {{TYPE_NAME}}& native{{INSTANCE_NAME:x-cap}});
};

} // namespace internal
} // namespace android
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}
{{/LITE}}{{/STRUCT}}{{#CLASSIC_LISTENER}}{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace android {
namespace internal {

template <>
struct ToNative<std::shared_ptr<{{TYPE_NAME}}>, jobject, void> {
    static std::shared_ptr<{{TYPE_NAME}}> from(
        jobject platform{{INSTANCE_NAME:x-cap}});
};
template <>
struct ToNative<std::shared_ptr<{{TYPE_NAME}}>, {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject> {
    static std::shared_ptr<{{TYPE_NAME}}> from(
        {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject platform{{INSTANCE_NAME:x-cap}})
    {
        return ToNative<std::shared_ptr<{{TYPE_NAME}}>, jobject>::from(
            platform{{INSTANCE_NAME:x-cap}}.get());
    }
};

template <>
struct ToPlatform<std::shared_ptr<{{TYPE_NAME}}>> {
    static JniObject from(
        const std::shared_ptr<{{TYPE_NAME}}>& native{{INSTANCE_NAME:x-cap}});
};

} // namespace internal
} // namespace android
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}
{{/CLASSIC_LISTENER}}