#include <test/docs/internal/android/test_binding.h>

namespace test {
namespace docs {
namespace android {

OnResponse createOnResponse(::yandex::maps::runtime::android::JniObject lambdaListener)
{
    return [lambdaListener](
        const std::shared_ptr<::test::docs::Response>& response)
    {
        if (!lambdaListener) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            ::yandex::maps::runtime::android::callMethod<void>(
                lambdaListener.get(),
                "onResponse",
                "(Lru/test/docs/Response;)V",
                ::yandex::maps::runtime::android::toJavaNoLocal(::yandex::maps::runtime::bindings::android::toPlatform(response)));
        });
    };
}

OnError createOnError(::yandex::maps::runtime::android::JniObject lambdaListener)
{
    return [lambdaListener](
        ::test::docs::TestError error)
    {
        if (!lambdaListener) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            ::yandex::maps::runtime::android::callMethod<void>(
                lambdaListener.get(),
                "onError",
                "(Lru/test/docs/TestError;)V",
                ::yandex::maps::runtime::android::toJavaNoLocal(::yandex::maps::runtime::bindings::android::toPlatform(error)));
        });
    };
}

} // namespace android
} // namespace docs
} // namespace test

namespace {

std::shared_ptr<::test::docs::Interface> nativeInterface(jobject self)
{
    return ::yandex::maps::runtime::android::weakGet<::test::docs::Interface>(self);
}

} // namespace

namespace {

::test::docs::InterfaceWithDocs* nativeInterfaceWithDocs(jobject self)
{
    return ::yandex::maps::runtime::android::uniqueGet<::test::docs::InterfaceWithDocs>(self);
}

} // namespace

namespace test {
namespace docs {

boost::any createPlatform(const std::shared_ptr<::test::docs::Interface>& interface)
{
    static const ::yandex::maps::runtime::android::JavaBindingFactory factory("ru/test/docs/internal/JavaInterfaceBinding");
    return factory(::yandex::maps::runtime::android::makeWeakObject<::test::docs::Interface>(interface).get());
}

} // namespace docs
} // namespace test

extern "C" {

JNIEXPORT void JNICALL
Java_ru_test_docs_internal_JavaInterfaceBinding_method__IFLru_test_docs_Struct_2Lru_test_docs_Variant_2(
    JNIEnv* env,
    jobject self,
    jint intValue,
    jfloat floatValue,
    jobject someStruct,
    jobject andVariant)
{
    BEGIN_NATIVE_FUNCTION;
    REQUIRE(someStruct, "Required method parameter \"someStruct\" cannot be null");
    REQUIRE(andVariant, "Required method parameter \"andVariant\" cannot be null");

    nativeInterface(self)->method(
        ::yandex::maps::runtime::bindings::android::toNative<int>(intValue),
        ::yandex::maps::runtime::bindings::android::toNative<float>(floatValue),
        ::yandex::maps::runtime::bindings::android::toNative<const ::test::docs::Struct>(someStruct),
        ::yandex::maps::runtime::bindings::android::toNative<::test::docs::Variant>(andVariant));

    END_NATIVE_FUNCTION(env);
}

JNIEXPORT jboolean JNICALL
Java_ru_test_docs_internal_JavaInterfaceBinding_isValid__(
    JNIEnv* env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION;

    auto nativeObject = ::yandex::maps::runtime::android::getLocalNativeObject(self);
    return ::yandex::maps::runtime::android::weakObject_cpp_cast<::test::docs::Interface>(nativeObject.get()) != nullptr;

    END_NATIVE_FUNCTION(env);
    return {};
}

JNIEXPORT jboolean JNICALL
Java_ru_test_docs_internal_InterfaceWithDocsBinding_methodWithDocs__Lru_test_docs_JavaInterface_2Lru_test_docs_Struct_2Lru_test_docs_LambdaListener_2(
    JNIEnv* env,
    jobject self,
    jobject i,
    jobject s,
    jobject l)
{
    BEGIN_NATIVE_FUNCTION;
    REQUIRE(s, "Required method parameter \"s\" cannot be null");

    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(nativeInterfaceWithDocs(self)->methodWithDocs(
        ::yandex::maps::runtime::bindings::android::toNative<::test::docs::Interface*>(i),
        ::yandex::maps::runtime::bindings::android::toNative<const ::test::docs::Struct>(s),
        test::docs::android::createOnResponse(l),
        test::docs::android::createOnError(l))));

    END_NATIVE_FUNCTION(env);
    return {};
}

} // extern "C"

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace android {
namespace internal {

YANDEX_EXPORT ::test::docs::Variant ToNative<::test::docs::Variant, jobject, void>::from(
    jobject platformVariant)
{
    auto platformI = ::yandex::maps::runtime::android::callMethod<::yandex::maps::runtime::android::JniObject>(
        platformVariant,
        "getI",
        "()I");
    if (platformI) {
        return ::yandex::maps::runtime::bindings::android::toNative<int>(platformI.get());
    }

    auto platformF = ::yandex::maps::runtime::android::callMethod<::yandex::maps::runtime::android::JniObject>(
        platformVariant,
        "getF",
        "()F");
    if (platformF) {
        return ::yandex::maps::runtime::bindings::android::toNative<float>(platformF.get());
    }

    throw ::yandex::maps::runtime::Exception("Invalid variant value");
}

YANDEX_EXPORT ::yandex::maps::runtime::android::JniObject ToPlatform<::test::docs::Variant>::from(
    const ::test::docs::Variant& variant)
{
    struct ToJavaVisitor : public boost::static_visitor<::yandex::maps::runtime::android::JniObject> {
        ::yandex::maps::runtime::android::JniObject operator()(const int& obj) const
        {
            return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::android::callStaticMethod<::yandex::maps::runtime::android::JniObject>(
                ::yandex::maps::runtime::android::findClass("ru/test/docs/Variant").get(),
                "fromI",
                "(I)Lru/test/docs/Variant;",
                ::yandex::maps::runtime::android::toJavaNoLocal(::yandex::maps::runtime::bindings::android::toPlatform(obj))));
        }

        ::yandex::maps::runtime::android::JniObject operator()(const float& obj) const
        {
            return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::android::callStaticMethod<::yandex::maps::runtime::android::JniObject>(
                ::yandex::maps::runtime::android::findClass("ru/test/docs/Variant").get(),
                "fromF",
                "(F)Lru/test/docs/Variant;",
                ::yandex::maps::runtime::android::toJavaNoLocal(::yandex::maps::runtime::bindings::android::toPlatform(obj))));
        }
    };

    return boost::apply_visitor(ToJavaVisitor(), variant);
}

} // namespace internal
} // namespace android
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

extern "C" {

JNIEXPORT jobject JNICALL Java_ru_test_docs_Response_init(
    JNIEnv* env,
    jobject /* this */,
    jint i,
    jfloat f)
{
    BEGIN_NATIVE_FUNCTION

    auto self = std::make_shared<::test::docs::Response>();
    self->i = ::yandex::maps::runtime::bindings::android::toNative<typename std::remove_reference<decltype(self->i)>::type>(i);
    self->f = ::yandex::maps::runtime::bindings::android::toNative<typename std::remove_reference<decltype(self->f)>::type>(f);

    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::android::makeSharedObject<::test::docs::Response>(self));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jint JNICALL Java_ru_test_docs_Response_getI_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = ::yandex::maps::runtime::android::sharedGet<::test::docs::Response>(self)->i;
    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jfloat JNICALL Java_ru_test_docs_Response_getF_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = ::yandex::maps::runtime::android::sharedGet<::test::docs::Response>(self)->f;
    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_Response_loadNative(
    JNIEnv* env,
    jclass /* cls */,
    jobject buffer)
{
    BEGIN_NATIVE_FUNCTION

    return ::yandex::maps::runtime::bindings::android::internal::deserializeNative<::test::docs::Response>(buffer);

    END_NATIVE_FUNCTION(env);
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_Response_saveNative(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    return ::yandex::maps::runtime::bindings::android::internal::serializeNative<::test::docs::Response>(self);

    END_NATIVE_FUNCTION(env);
    return {};
}

} // extern "C"

template YANDEX_EXPORT boost::any yandex::maps::runtime::any::internal::BridgedHolder<::test::docs::Response>::platformObject() const;


namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace android {
namespace internal {

namespace {

struct StructIFieldTrait {
    static constexpr const char* const name = "i";
    static constexpr const char* const typeName = "I";
    using type = int;
};

} // namespace

YANDEX_EXPORT ::test::docs::Struct ToNative<::test::docs::Struct, jobject, void>::from(
    jobject platformStruct)
{
    return ::test::docs::Struct(
        extractField<::test::docs::Struct, StructIFieldTrait>(platformStruct));
}

YANDEX_EXPORT JniObject ToPlatform<::test::docs::Struct>::from(
    const ::test::docs::Struct& nativeStruct)
{
    static const auto CLASS = runtime::android::findClass(runtime::bindings::BindingTraits<::test::docs::Struct>::javaUndecoratedName);
    static const auto CONSTRUCTOR = runtime::android::constructor(CLASS.get(), "(I)V");
    return runtime::android::createObject(
        CLASS.get(),
        CONSTRUCTOR,
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeStruct.i))
    );
}

} // namespace internal
} // namespace android
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace android {
namespace internal {

namespace {

struct OptionsStructureEmptyFieldTrait {
    static constexpr const char* const name = "empty";
    static constexpr const char* const typeName = "Ljava/lang/String;";
    using type = std::string;
};

struct OptionsStructureFilledFieldTrait {
    static constexpr const char* const name = "filled";
    static constexpr const char* const typeName = "Ljava/lang/String;";
    using type = std::string;
};

} // namespace

YANDEX_EXPORT ::test::docs::OptionsStructure ToNative<::test::docs::OptionsStructure, jobject, void>::from(
    jobject platformOptionsStructure)
{
    return ::test::docs::OptionsStructure(
        extractField<::test::docs::OptionsStructure, OptionsStructureEmptyFieldTrait>(platformOptionsStructure),
        extractField<::test::docs::OptionsStructure, OptionsStructureFilledFieldTrait>(platformOptionsStructure));
}

YANDEX_EXPORT JniObject ToPlatform<::test::docs::OptionsStructure>::from(
    const ::test::docs::OptionsStructure& nativeOptionsStructure)
{
    static const auto CLASS = runtime::android::findClass(runtime::bindings::BindingTraits<::test::docs::OptionsStructure>::javaUndecoratedName);
    static const auto CONSTRUCTOR = runtime::android::constructor(CLASS.get(), "(Ljava/lang/String;Ljava/lang/String;)V");
    return runtime::android::createObject(
        CLASS.get(),
        CONSTRUCTOR,
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeOptionsStructure.empty())),
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeOptionsStructure.filled()))
    );
}

} // namespace internal
} // namespace android
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace android {
namespace internal {

namespace {

struct DefaultValueFilledFieldTrait {
    static constexpr const char* const name = "filled";
    static constexpr const char* const typeName = "Ljava/lang/String;";
    using type = std::string;
};

} // namespace

YANDEX_EXPORT ::test::docs::DefaultValue ToNative<::test::docs::DefaultValue, jobject, void>::from(
    jobject platformDefaultValue)
{
    return ::test::docs::DefaultValue(
        extractField<::test::docs::DefaultValue, DefaultValueFilledFieldTrait>(platformDefaultValue));
}

YANDEX_EXPORT JniObject ToPlatform<::test::docs::DefaultValue>::from(
    const ::test::docs::DefaultValue& nativeDefaultValue)
{
    static const auto CLASS = runtime::android::findClass(runtime::bindings::BindingTraits<::test::docs::DefaultValue>::javaUndecoratedName);
    static const auto CONSTRUCTOR = runtime::android::constructor(CLASS.get(), "(Ljava/lang/String;)V");
    return runtime::android::createObject(
        CLASS.get(),
        CONSTRUCTOR,
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeDefaultValue.filled))
    );
}

} // namespace internal
} // namespace android
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace android {
namespace internal {

namespace {

struct CombinedValuesEmptyFieldTrait {
    static constexpr const char* const name = "empty";
    static constexpr const char* const typeName = "Ljava/lang/String;";
    using type = std::string;
};

struct CombinedValuesFilledFieldTrait {
    static constexpr const char* const name = "filled";
    static constexpr const char* const typeName = "Ljava/lang/String;";
    using type = std::string;
};

} // namespace

YANDEX_EXPORT ::test::docs::CombinedValues ToNative<::test::docs::CombinedValues, jobject, void>::from(
    jobject platformCombinedValues)
{
    return ::test::docs::CombinedValues(
        extractField<::test::docs::CombinedValues, CombinedValuesEmptyFieldTrait>(platformCombinedValues),
        extractField<::test::docs::CombinedValues, CombinedValuesFilledFieldTrait>(platformCombinedValues));
}

YANDEX_EXPORT JniObject ToPlatform<::test::docs::CombinedValues>::from(
    const ::test::docs::CombinedValues& nativeCombinedValues)
{
    static const auto CLASS = runtime::android::findClass(runtime::bindings::BindingTraits<::test::docs::CombinedValues>::javaUndecoratedName);
    static const auto CONSTRUCTOR = runtime::android::constructor(CLASS.get(), "(Ljava/lang/String;Ljava/lang/String;)V");
    return runtime::android::createObject(
        CLASS.get(),
        CONSTRUCTOR,
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeCombinedValues.empty)),
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeCombinedValues.filled))
    );
}

} // namespace internal
} // namespace android
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex

namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace android {
namespace internal {

namespace {

struct DefaultTimeintervalValueEmptyFieldTrait {
    static constexpr const char* const name = "empty";
    static constexpr const char* const typeName = "J";
    using type = ::yandex::maps::runtime::TimeInterval;
};

struct DefaultTimeintervalValueFilledFieldTrait {
    static constexpr const char* const name = "filled";
    static constexpr const char* const typeName = "J";
    using type = ::yandex::maps::runtime::TimeInterval;
};

} // namespace

YANDEX_EXPORT ::test::docs::DefaultTimeintervalValue ToNative<::test::docs::DefaultTimeintervalValue, jobject, void>::from(
    jobject platformDefaultTimeintervalValue)
{
    return ::test::docs::DefaultTimeintervalValue(
        extractField<::test::docs::DefaultTimeintervalValue, DefaultTimeintervalValueEmptyFieldTrait>(platformDefaultTimeintervalValue),
        extractField<::test::docs::DefaultTimeintervalValue, DefaultTimeintervalValueFilledFieldTrait>(platformDefaultTimeintervalValue));
}

YANDEX_EXPORT JniObject ToPlatform<::test::docs::DefaultTimeintervalValue>::from(
    const ::test::docs::DefaultTimeintervalValue& nativeDefaultTimeintervalValue)
{
    static const auto CLASS = runtime::android::findClass(runtime::bindings::BindingTraits<::test::docs::DefaultTimeintervalValue>::javaUndecoratedName);
    static const auto CONSTRUCTOR = runtime::android::constructor(CLASS.get(), "(JJ)V");
    return runtime::android::createObject(
        CLASS.get(),
        CONSTRUCTOR,
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeDefaultTimeintervalValue.empty)),
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeDefaultTimeintervalValue.filled))
    );
}

} // namespace internal
} // namespace android
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex
