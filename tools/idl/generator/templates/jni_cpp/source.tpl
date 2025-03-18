{{#HEADER_WILL_BE_GENERATED}}#include <{{OWN_HEADER_INCLUDE_PATH}}>{{/HEADER_WILL_BE_GENERATED}}{{#HEADER_WILL_NOT_BE_GENERATED}}{{#IMPORT_SECTION}}{{#IMPORT}}#include {{IMPORT_PATH}}{{#IMPORT_separator}}
{{/IMPORT_separator}}{{/IMPORT}}{{#IMPORT_SECTION_separator}}

{{/IMPORT_SECTION_separator}}{{/IMPORT_SECTION}}{{/HEADER_WILL_NOT_BE_GENERATED}}{{#PUBLIC_METHODS}}{{#CLASSIC_LISTENER}}

{{#NAMESPACE}}namespace {{NAME}} {
{{/NAMESPACE}}namespace android {

{{CONSTRUCTOR_NAME}}Binding::{{CONSTRUCTOR_NAME}}Binding({{RUNTIME_NAMESPACE_PREFIX}}android::JniObject {{INSTANCE_NAME}})
    : {{INSTANCE_NAME}}_({{#WEAK_LISTENER}}{{RUNTIME_NAMESPACE_PREFIX}}android::makeJniWeak({{/WEAK_LISTENER}}{{INSTANCE_NAME}}{{#WEAK_LISTENER}}.get()){{/WEAK_LISTENER}})
{
}{{#METHOD}}

{{RESULT_TYPE}} {{CONSTRUCTOR_NAME}}Binding::{{FUNCTION_NAME}}({{#PARAMETER}}{{#PARAM}}
    {{#CONST_PARAM}}const {{/CONST_PARAM}}{{PARAM_TYPE}}{{#REF_PARAM}}&{{/REF_PARAM}} {{PARAM_NAME}}{{/PARAM}}{{#PARAMETER_separator}},{{/PARAMETER_separator}}{{/PARAMETER}}){{#CONST_METHOD}} const{{/CONST_METHOD}}
{
    return {{RUNTIME_NAMESPACE_PREFIX}}verify{{#UI_THREAD}}Ui{{/UI_THREAD}}{{#BG_THREAD}}BgPlatform{{/BG_THREAD}}{{#ANY_THREAD}}AnyThread{{/ANY_THREAD}}AndRun([&] {
        static const jmethodID JNI_METHOD_ID =
            {{RUNTIME_NAMESPACE_PREFIX}}android::methodID(JNI_TYPE_REF, "{{JNI_FUNCTION_NAME}}",
                "({{#PARAMETER}}{{#PARAM}}{{JNI_PARAM_TYPE_REF}}{{/PARAM}}{{/PARAMETER}}){{JNI_RESULT_TYPE_REF}}");

        {{#RETURNS_SOMETHING}}return {{#SIMPLE}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toNative<{{RESULT_TYPE}}>{{/SIMPLE}}{{#LISTENER}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toNative<{{RESULT_TYPE}}>{{/LISTENER}}({{/RETURNS_SOMETHING}}{{RUNTIME_NAMESPACE_PREFIX}}android::{{#STRONG_LISTENER}}callMethod{{/STRONG_LISTENER}}{{#WEAK_LISTENER}}tryCall{{/WEAK_LISTENER}}<{{JNI_RESULT_GLOBAL_REF_TYPE_NAME}}>(
            {{INSTANCE_NAME}}_{{#STRONG_LISTENER}}.get(){{/STRONG_LISTENER}},
            JNI_METHOD_ID{{#PARAMETER}},{{#SIMPLE}}{{#PARAM}}
            {{RUNTIME_NAMESPACE_PREFIX}}android::toJavaNoLocal({{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toPlatform({{#MOVE}}std::move({{/MOVE}}{{PARAM_NAME}}{{#MOVE}}){{/MOVE}})){{/PARAM}}{{/SIMPLE}}{{#LISTENER}}{{#PARAM}}
            {{RUNTIME_NAMESPACE_PREFIX}}android::toJavaNoLocal({{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toPlatform({{PARAM_NAME}})){{/PARAM}}{{/LISTENER}}{{#ERROR}}{{#PARAM}}
            {{RUNTIME_NAMESPACE_PREFIX}}makeError({{PARAM_NAME}}).get(){{/PARAM}}{{/ERROR}}{{/PARAMETER}}){{#RETURNS_SOMETHING}}){{/RETURNS_SOMETHING}};
    });
}{{/METHOD}}

const std::string {{CONSTRUCTOR_NAME}}Binding::JNI_TYPE_REF =
    "{{LISTENER_BINDING_TYPE_REF}}";

} // namespace android{{#CLOSING_NAMESPACE}}
} // namespace {{NAME}}{{/CLOSING_NAMESPACE}}{{/CLASSIC_LISTENER}}{{#LAMBDA_LISTENER}}

{{#NAMESPACE}}namespace {{NAME}} {
{{/NAMESPACE}}namespace android {{{#METHOD}}

{{SCOPE_PREFIX}}{{JNI_FUNCTION_NAME:x-cap}} create{{JNI_FUNCTION_NAME:x-cap}}({{RUNTIME_NAMESPACE_PREFIX}}android::JniObject {{INSTANCE_NAME}})
{
    return [{{INSTANCE_NAME}}]({{#PARAMETER}}{{#PARAM}}
        {{#CONST_PARAM}}const {{/CONST_PARAM}}{{PARAM_TYPE}}{{#REF_PARAM}}&{{/REF_PARAM}} {{PARAM_NAME}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}}{{#PARAMETER_separator}},{{/PARAMETER_separator}}{{/PARAMETER}})
    {
        if (!{{INSTANCE_NAME}}) {
            return;
        }

        {{RUNTIME_NAMESPACE_PREFIX}}verify{{#UI_THREAD}}Ui{{/UI_THREAD}}{{#BG_THREAD}}BgPlatform{{/BG_THREAD}}{{#ANY_THREAD}}AnyThread{{/ANY_THREAD}}AndRun([&] {
            {{RUNTIME_NAMESPACE_PREFIX}}android::callMethod<{{JNI_RESULT_GLOBAL_REF_TYPE_NAME}}>(
                {{INSTANCE_NAME}}.get(),
                "{{JNI_FUNCTION_NAME:x-uncap}}",
                "({{#PARAMETER}}{{#SIMPLE}}{{#PARAM}}{{JNI_PARAM_TYPE_REF}}{{/PARAM}}{{/SIMPLE}}{{#ERROR}}{{#PARAM}}{{JNI_PARAM_TYPE_REF}}{{/PARAM}}{{/ERROR}}{{/PARAMETER}}){{JNI_RESULT_TYPE_REF}}"{{#PARAMETER}},{{#SIMPLE}}{{#PARAM}}
                {{RUNTIME_NAMESPACE_PREFIX}}android::toJavaNoLocal({{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toPlatform({{#MOVE}}std::move({{/MOVE}}{{PARAM_NAME}}{{#MOVE}}){{/MOVE}})){{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}}{{/SIMPLE}}{{#LISTENER}}{{#PARAM}}
                {{RUNTIME_NAMESPACE_PREFIX}}android::toJavaNoLocal({{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toPlatform({{PARAM_NAME}})){{/PARAM}}{{/LISTENER}}{{#ERROR}}{{#PARAM}}
                {{RUNTIME_NAMESPACE_PREFIX}}makeError({{PARAM_NAME}}).get(){{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}}{{/ERROR}}{{/PARAMETER}}){{#RETURNS_SOMETHING}}){{/RETURNS_SOMETHING}};
        });
    };
}{{/METHOD}}

} // namespace android{{#CLOSING_NAMESPACE}}
} // namespace {{NAME}}{{/CLOSING_NAMESPACE}}{{/LAMBDA_LISTENER}}{{/PUBLIC_METHODS}}{{#INTERFACES}}{{#INTERFACE}}{{#IS_NOT_STATIC}}

namespace {{{#STRONG_INTERFACE}}

{{TYPE_NAME}}* native{{INSTANCE_NAME:x-cap}}(jobject self)
{
    return {{#HAS_PARENT_INTERFACE}}dynamic_cast<{{TYPE_NAME}}*>(
        {{/HAS_PARENT_INTERFACE}}{{RUNTIME_NAMESPACE_PREFIX}}android::uniqueGet<{{TOP_MOST_BASE_TYPE_NAME}}>(self){{#HAS_PARENT_INTERFACE}}){{/HAS_PARENT_INTERFACE}};
}{{/STRONG_INTERFACE}}{{#NOT_STRONG_INTERFACE}}

std::shared_ptr<{{TYPE_NAME}}> native{{INSTANCE_NAME:x-cap}}(jobject self)
{
    return {{#HAS_PARENT_INTERFACE}}std::dynamic_pointer_cast<{{TYPE_NAME}}>(
        {{/HAS_PARENT_INTERFACE}}{{RUNTIME_NAMESPACE_PREFIX}}android::{{#SHARED_INTERFACE}}shared{{/SHARED_INTERFACE}}{{#WEAK_INTERFACE}}weak{{/WEAK_INTERFACE}}Get<{{TOP_MOST_BASE_TYPE_NAME}}>(self){{#HAS_PARENT_INTERFACE}}){{/HAS_PARENT_INTERFACE}};
}{{/NOT_STRONG_INTERFACE}}

} // namespace{{/IS_NOT_STATIC}}{{/INTERFACE}}{{#INTERFACE}}{{#IS_NOT_STATIC}}{{#WEAK_INTERFACE}}
{{#NAMESPACE}}
namespace {{NAME}} {{{/NAMESPACE}}

boost::any createPlatform(const std::shared_ptr<{{TYPE_NAME}}>& {{INSTANCE_NAME}})
{
    static const {{RUNTIME_NAMESPACE_PREFIX}}android::JavaBindingFactory factory("{{INTERFACE_BINDING_TYPE_REF}}");
    return factory({{RUNTIME_NAMESPACE_PREFIX}}android::makeWeakObject<{{TOP_MOST_BASE_TYPE_NAME}}>({{INSTANCE_NAME}}).get());
}
{{#CLOSING_NAMESPACE}}
} // namespace {{NAME}}{{/CLOSING_NAMESPACE}}{{/WEAK_INTERFACE}}{{/IS_NOT_STATIC}}{{/INTERFACE}}

extern "C" {{{#INTERFACE}}{{#ITEM}}{{#METHOD}}

JNIEXPORT {{JNI_RESULT_TYPE_NAME}} JNICALL
Java_{{#IS_STATIC}}{{INTERFACE_NAME_IN_FUNCTION_NAME}}{{/IS_STATIC}}{{#IS_NOT_STATIC}}{{INTERFACE_BINDING_NAME_IN_FUNCTION_NAME}}{{/IS_NOT_STATIC}}_{{JNI_FUNCTION_NAME}}{{JNI_PARAM_TYPE_REFS_IN_FUNCTION_NAME}}(
    JNIEnv* env,
    {{#IS_STATIC}}jclass /* cls */{{/IS_STATIC}}{{#IS_NOT_STATIC}}jobject self{{/IS_NOT_STATIC}}{{#PARAMETER}}{{#JNI_PARAM}},
    {{PARAM_TYPE}} {{PARAM_NAME}}{{/JNI_PARAM}}{{/PARAMETER}})
{
    BEGIN_NATIVE_FUNCTION;{{#JAVA_NULL_ASSERT}}
    REQUIRE({{FIELD_NAME:x-uncap}}, "Required method parameter \"{{FIELD_NAME}}\" cannot be null");{{/JAVA_NULL_ASSERT}}

    {{#RETURNS_SOMETHING}}return {{RUNTIME_NAMESPACE_PREFIX}}android::toJava({{#SIMPLE}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toPlatform({{/SIMPLE}}{{#LISTENER}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toPlatform({{/LISTENER}}{{/RETURNS_SOMETHING}}{{#IS_STATIC}}::{{#NAMESPACE}}{{NAME}}::{{/NAMESPACE}}{{/IS_STATIC}}{{#IS_NOT_STATIC}}native{{INSTANCE_NAME:x-cap}}(self)->{{/IS_NOT_STATIC}}{{FUNCTION_NAME}}({{#PARAMETER}}{{#LISTENER}}{{#WEAK_LISTENER_PARAM}}{{#PARAM}}
        {{PARAM_NAME}} == nullptr ? nullptr : {{RUNTIME_NAMESPACE_PREFIX}}android::sharedObject_cpp_cast<{{PARAM_TYPE:x-strip-shared-ptr}}>(
            {{RUNTIME_NAMESPACE_PREFIX}}android::getSubscribedListener(self, "{{LISTENER_NAME:x-uncap}}Subscription", {{PARAM_NAME}}).get()){{/PARAM}}{{/WEAK_LISTENER_PARAM}}{{#STRONG_LISTENER_PARAM}}{{#PARAM}}
        {{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toNative<{{PARAM_TYPE}}>({{JNI_PARAM_NAME}}){{/PARAM}}{{/STRONG_LISTENER_PARAM}}{{/LISTENER}}{{#LAMBDA}}{{#PARAM}}
        {{LISTENER_SCOPE}}android::create{{PARAM_NAME:x-cap}}({{JNI_PARAM_NAME}}){{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}}{{/LAMBDA}}{{#ERROR}}{{#PARAM}}
        {{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toNative<{{PARAM_TYPE}}>({{JNI_PARAM_NAME}}){{/PARAM}}{{/ERROR}}{{#SIMPLE}}{{#PARAM}}
        {{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toNative<{{PARAM_TYPE}}>({{JNI_PARAM_NAME}}){{/PARAM}}{{/SIMPLE}}{{#PARAMETER_separator}},{{/PARAMETER_separator}}{{/PARAMETER}}){{#RETURNS_SOMETHING}}{{#SIMPLE}}){{/SIMPLE}}{{#LISTENER}}){{/LISTENER}}){{/RETURNS_SOMETHING}};

    END_NATIVE_FUNCTION(env);{{#RETURNS_SOMETHING}}
    return {};{{/RETURNS_SOMETHING}}
}{{/METHOD}}{{#PROPERTY}}

JNIEXPORT {{JNI_PROPERTY_TYPE_NAME}} JNICALL
Java_{{#IS_STATIC}}{{INTERFACE_NAME_IN_FUNCTION_NAME}}{{/IS_STATIC}}{{#IS_NOT_STATIC}}{{INTERFACE_BINDING_NAME_IN_FUNCTION_NAME}}{{/IS_NOT_STATIC}}_{{#BOOL}}is{{/BOOL}}{{#NOT_BOOL}}get{{/NOT_BOOL}}{{PROPERTY_NAME:x-cap}}__(
    JNIEnv* env,
    {{#IS_STATIC}}jclass /* cls */{{/IS_STATIC}}{{#IS_NOT_STATIC}}jobject self{{/IS_NOT_STATIC}})
{
    BEGIN_NATIVE_FUNCTION;

    return {{RUNTIME_NAMESPACE_PREFIX}}android::toJava({{#LISTENER}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toPlatform(
        {{/LISTENER}}{{#NOT_LISTENER}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toPlatform(
        {{/NOT_LISTENER}}{{#IS_STATIC}}::{{#NAMESPACE}}{{NAME}}::{{/NAMESPACE}}{{/IS_STATIC}}{{#IS_NOT_STATIC}}native{{INSTANCE_NAME:x-cap}}(self)->{{/IS_NOT_STATIC}}{{PROPERTY_NAME}}()));

    END_NATIVE_FUNCTION(env);
    return {};
}{{#NOT_INTERFACE}}{{#NOT_READONLY}}

JNIEXPORT void JNICALL
Java_{{#IS_STATIC}}{{INTERFACE_NAME_IN_FUNCTION_NAME}}{{/IS_STATIC}}{{#IS_NOT_STATIC}}{{INTERFACE_BINDING_NAME_IN_FUNCTION_NAME}}{{/IS_NOT_STATIC}}_set{{PROPERTY_NAME:x-cap}}__{{JNI_PROPERTY_TYPE_REF_IN_SETTER_NAME}}(
    JNIEnv* env,
    {{#IS_STATIC}}jclass /* cls */{{/IS_STATIC}}{{#IS_NOT_STATIC}}jobject self{{/IS_NOT_STATIC}},
    {{JNI_PROPERTY_TYPE_NAME}} {{PROPERTY_NAME}})
{
    BEGIN_NATIVE_FUNCTION;

    {{#JAVA_NULL_ASSERT}}REQUIRE({{FIELD_NAME:x-uncap}}, "Required property setter parameter \"{{FIELD_NAME}}\" cannot be null");

    {{/JAVA_NULL_ASSERT}}{{#IS_STATIC}}::{{#NAMESPACE}}{{NAME}}::{{/NAMESPACE}}{{/IS_STATIC}}{{#IS_NOT_STATIC}}native{{INSTANCE_NAME:x-cap}}(self)->{{/IS_NOT_STATIC}}set{{PROPERTY_NAME:x-cap}}({{#LISTENER}}{{#WEAK_LISTENER_PROPERTY}}
        {{PROPERTY_NAME}} == nullptr ? nullptr : {{RUNTIME_NAMESPACE_PREFIX}}android::sharedObject_cpp_cast<{{PROPERTY_TYPE:x-strip-shared-ptr}}>(
            {{RUNTIME_NAMESPACE_PREFIX}}android::getSubscribedListener(self, "{{PROPERTY_NAME:x-uncap}}Subscription", {{PROPERTY_NAME}}).get()){{/WEAK_LISTENER_PROPERTY}}{{#STRONG_LISTENER_PROPERTY}}
        {{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toNative<{{PROPERTY_TYPE}}>({{PROPERTY_NAME}}){{/STRONG_LISTENER_PROPERTY}}{{/LISTENER}}{{#NOT_LISTENER}}
        {{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toNative<{{PROPERTY_TYPE}}>({{PROPERTY_NAME}}){{/NOT_LISTENER}});

    END_NATIVE_FUNCTION(env);
}{{/NOT_READONLY}}{{/NOT_INTERFACE}}{{/PROPERTY}}{{/ITEM}}{{#IS_NOT_STATIC}}{{#WEAK_INTERFACE}}{{#NO_PARENT}}

JNIEXPORT jboolean JNICALL
Java_{{INTERFACE_BINDING_NAME_IN_FUNCTION_NAME}}_isValid__(
    JNIEnv* env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION;

    auto nativeObject = {{RUNTIME_NAMESPACE_PREFIX}}android::getLocalNativeObject(self);
    return {{RUNTIME_NAMESPACE_PREFIX}}android::weakObject_cpp_cast<{{TOP_MOST_BASE_TYPE_NAME}}>(nativeObject.get()) != nullptr;

    END_NATIVE_FUNCTION(env);
    return {};
}{{/NO_PARENT}}{{/WEAK_INTERFACE}}{{#SUBSCRIPTION}}

JNIEXPORT jobject JNICALL
Java_{{INTERFACE_BINDING_NAME_IN_FUNCTION_NAME}}_create{{LISTENER_NAME:x-cap}}(
    JNIEnv* env,
    jclass /* cls */,
    jobject {{LISTENER_NAME:x-uncap}})
{
    BEGIN_NATIVE_FUNCTION;

    return {{RUNTIME_NAMESPACE_PREFIX}}android::toJava(
        {{RUNTIME_NAMESPACE_PREFIX}}android::makeSharedObject<{{LISTENER_TYPE_NAME}}>(
            {{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toNative<std::shared_ptr<{{LISTENER_TYPE_NAME}}>>({{LISTENER_NAME:x-uncap}})));

    END_NATIVE_FUNCTION(env);
    return {};
}{{/SUBSCRIPTION}}{{/IS_NOT_STATIC}}{{/INTERFACE}}

} // extern "C"{{/INTERFACES}}{{#VARIANT}}

{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace android {
namespace internal {

YANDEX_EXPORT {{TYPE_NAME}} ToNative<{{TYPE_NAME}}, jobject, void>::from(
    jobject platform{{INSTANCE_NAME:x-cap}})
{{{#TYPES}}
    auto platform{{FIELD_NAME:x-cap}} = {{RUNTIME_NAMESPACE_PREFIX}}android::callMethod<{{RUNTIME_NAMESPACE_PREFIX}}android::JniObject>(
        platform{{INSTANCE_NAME:x-cap}},
        "get{{FIELD_NAME:x-cap}}",
        "(){{JNI_CLASS_TYPE}}");
    if (platform{{FIELD_NAME:x-cap}}) {
        return {{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toNative<{{CLASS_TYPE}}>(platform{{FIELD_NAME:x-cap}}.get());
    }
{{/TYPES}}
    throw {{RUNTIME_NAMESPACE_PREFIX}}Exception("Invalid variant value");
}

YANDEX_EXPORT {{RUNTIME_NAMESPACE_PREFIX}}android::JniObject ToPlatform<{{TYPE_NAME}}>::from(
    const {{TYPE_NAME}}& {{INSTANCE_NAME}})
{
    struct ToJavaVisitor : public boost::static_visitor<{{RUNTIME_NAMESPACE_PREFIX}}android::JniObject> {
        {{#TYPES}}{{RUNTIME_NAMESPACE_PREFIX}}android::JniObject operator()(const {{CLASS_TYPE}}& obj) const
        {
            return {{RUNTIME_NAMESPACE_PREFIX}}android::toJava({{RUNTIME_NAMESPACE_PREFIX}}android::callStaticMethod<{{RUNTIME_NAMESPACE_PREFIX}}android::JniObject>(
                {{RUNTIME_NAMESPACE_PREFIX}}android::findClass("{{JNI_VARIANT_NAME}}").get(),
                "from{{FIELD_NAME:x-cap}}",
                "({{JNI_CLASS_TYPE}})L{{JNI_VARIANT_NAME}};",
                {{RUNTIME_NAMESPACE_PREFIX}}android::toJavaNoLocal({{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toPlatform(obj))));
        }{{#TYPES_separator}}

        {{/TYPES_separator}}{{/TYPES}}
    };

    return boost::apply_visitor(ToJavaVisitor(), {{INSTANCE_NAME}});
}

} // namespace internal
} // namespace android
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}{{/VARIANT}}{{#STRUCT}}{{#BRIDGED}}

extern "C" {

JNIEXPORT jobject JNICALL Java_{{STRUCT_NAME_IN_FUNCTION_NAME}}_init(
    JNIEnv* env,
    jobject /* this */,{{#FIELD}}
    {{#VISIBLE_FIELD}}{{JNI_FIELD_TYPE}} {{FIELD_NAME}}{{/VISIBLE_FIELD}}{{#FIELD_SEP}},{{/FIELD_SEP}}{{/FIELD}})
{
    BEGIN_NATIVE_FUNCTION

    auto self = std::make_shared<{{TYPE_NAME}}>();{{#FIELD}}
    self->{{FIELD_NAME}} = {{#HIDDEN_FIELD}}{{VALUE}};{{/HIDDEN_FIELD}}{{#VISIBLE_FIELD}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toNative<typename std::remove_reference<decltype(self->{{FIELD_NAME}})>::type>({{FIELD_NAME}});{{/VISIBLE_FIELD}}{{/FIELD}}

    return {{RUNTIME_NAMESPACE_PREFIX}}android::toJava({{RUNTIME_NAMESPACE_PREFIX}}android::makeSharedObject<{{TYPE_NAME}}>(self));

    END_NATIVE_FUNCTION(env)
    return {};
}

{{#FIELD}}JNIEXPORT {{JNI_FIELD_TYPE}} JNICALL Java_{{STRUCT_NAME_IN_FUNCTION_NAME}}_get{{FIELD_NAME:x-cap}}_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = {{RUNTIME_NAMESPACE_PREFIX}}android::sharedGet<{{TYPE_NAME}}>(self)->{{FIELD_NAME}};
    return {{RUNTIME_NAMESPACE_PREFIX}}android::toJava({{RUNTIME_NAMESPACE_PREFIX}}bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

{{/FIELD}}JNIEXPORT jobject JNICALL Java_{{STRUCT_NAME_IN_FUNCTION_NAME}}_loadNative(
    JNIEnv* env,
    jclass /* cls */,
    jobject buffer)
{
    BEGIN_NATIVE_FUNCTION

    return {{RUNTIME_NAMESPACE_PREFIX}}bindings::android::internal::deserializeNative<{{TYPE_NAME}}>(buffer);

    END_NATIVE_FUNCTION(env);
    return {};
}

JNIEXPORT jobject JNICALL Java_{{STRUCT_NAME_IN_FUNCTION_NAME}}_saveNative(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    return {{RUNTIME_NAMESPACE_PREFIX}}bindings::android::internal::serializeNative<{{TYPE_NAME}}>(self);

    END_NATIVE_FUNCTION(env);
    return {};
}

} // extern "C"

template YANDEX_EXPORT boost::any {{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}{{NAME}}::{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}any::internal::BridgedHolder<{{TYPE_NAME}}>::platformObject() const;
{{/BRIDGED}}{{#VISIBLE_PLATFORM_OBJECT}}
template boost::any {{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}{{NAME}}::{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}any::internal::BridgedHolder<{{TYPE_NAME}}>::platformObject() const;{{/VISIBLE_PLATFORM_OBJECT}}{{/STRUCT}}{{#STRUCT}}{{#LITE}}

{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace android {
namespace internal {

namespace {
{{#FIELD}}{{#VISIBLE_FIELD}}
struct {{CONSTRUCTOR_NAME}}{{FIELD_NAME:x-cap}}FieldTrait {
    static constexpr const char* const name = "{{FIELD_NAME}}";
    static constexpr const char* const typeName = "{{JNI_CLASS_TYPE}}";
    using type = {{FIELD_TYPE}};
};
{{/VISIBLE_FIELD}}{{/FIELD}}
} // namespace

YANDEX_EXPORT {{TYPE_NAME}} ToNative<{{TYPE_NAME}}, jobject, void>::from(
    jobject platform{{INSTANCE_NAME:x-cap}})
{
    return {{TYPE_NAME}}({{#FIELD}}{{#HIDDEN_FIELD}}
        {{VALUE}}{{/HIDDEN_FIELD}}{{#VISIBLE_FIELD}}
        extractField<{{TYPE_NAME}}, {{CONSTRUCTOR_NAME}}{{FIELD_NAME:x-cap}}FieldTrait>(platform{{INSTANCE_NAME:x-cap}}){{/VISIBLE_FIELD}}{{#FIELD_separator}},{{/FIELD_separator}}{{/FIELD}});
}

YANDEX_EXPORT JniObject ToPlatform<{{TYPE_NAME}}>::from(
    const {{TYPE_NAME}}& native{{INSTANCE_NAME:x-cap}})
{
    static const auto CLASS = runtime::android::findClass(runtime::bindings::BindingTraits<{{TYPE_NAME}}>::javaUndecoratedName);
    static const auto CONSTRUCTOR = runtime::android::constructor(CLASS.get(), "({{#FIELD}}{{JNI_CLASS_TYPE}}{{/FIELD}})V");
    return runtime::android::createObject(
        CLASS.get(),
        CONSTRUCTOR,{{#FIELD}}{{#VISIBLE_FIELD}}
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(native{{INSTANCE_NAME:x-cap}}.{{FIELD_NAME}}{{#IS_OPTIONS}}(){{/IS_OPTIONS}})){{/VISIBLE_FIELD}}{{#FIELD_SEP}},{{/FIELD_SEP}}{{/FIELD}}
    );
}

} // namespace internal
} // namespace android
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}{{/LITE}}{{/STRUCT}}{{#CLASSIC_LISTENER}}

{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}namespace {{NAME}} {{{#NAMESPACE_separator}}
{{/NAMESPACE_separator}}{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {
namespace android {
namespace internal {

YANDEX_EXPORT std::shared_ptr<{{TYPE_NAME}}> ToNative<std::shared_ptr<{{TYPE_NAME}}>, jobject, void>::from(
    jobject platform{{INSTANCE_NAME:x-cap}})
{
    if (!platform{{INSTANCE_NAME:x-cap}}) {
        return { };
    }

    return std::make_shared<{{LISTENER_BINDING_TYPE_NAME}}>(
        platform{{INSTANCE_NAME:x-cap}});
}

YANDEX_EXPORT JniObject ToPlatform<std::shared_ptr<{{TYPE_NAME}}>>::from(
    const std::shared_ptr<{{TYPE_NAME}}>& native{{INSTANCE_NAME:x-cap}})
{
    if (!native{{INSTANCE_NAME:x-cap}}) {
        return { };
    }

    return static_cast<{{LISTENER_BINDING_TYPE_NAME}}*>(native{{INSTANCE_NAME:x-cap}}.get())->platformReference();
}

} // namespace internal
} // namespace android
} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}{{/CLASSIC_LISTENER}}
