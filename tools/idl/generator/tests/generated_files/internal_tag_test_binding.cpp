#include <test/docs/internal/android/internal_tag_test_binding.h>

namespace test {
namespace docs {
namespace android {

FirstCallback createFirstCallback(::yandex::maps::runtime::android::JniObject soSecret)
{
    return [soSecret](
        const std::shared_ptr<::test::docs::VeryPrivate>& muchClassified)
    {
        if (!soSecret) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            ::yandex::maps::runtime::android::callMethod<void>(
                soSecret.get(),
                "firstCallback",
                "(Lru/test/docs/VeryPrivate;)V",
                ::yandex::maps::runtime::android::toJavaNoLocal(::yandex::maps::runtime::bindings::android::toPlatform(muchClassified)));
        });
    };
}

} // namespace android
} // namespace docs
} // namespace test

namespace test {
namespace docs {
namespace android {

CantMarkMethodsAsInternalHereYet createCantMarkMethodsAsInternalHereYet(::yandex::maps::runtime::android::JniObject soDeclassified)
{
    return [soDeclassified](
        const std::shared_ptr<::test::docs::VeryOpen>& knownStructure)
    {
        if (!soDeclassified) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            ::yandex::maps::runtime::android::callMethod<void>(
                soDeclassified.get(),
                "cantMarkMethodsAsInternalHereYet",
                "(Lru/test/docs/VeryOpen;)V",
                ::yandex::maps::runtime::android::toJavaNoLocal(::yandex::maps::runtime::bindings::android::toPlatform(knownStructure)));
        });
    };
}

} // namespace android
} // namespace docs
} // namespace test

namespace test {
namespace docs {
namespace android {

OnEmpty createOnEmpty(::yandex::maps::runtime::android::JniObject emptyCallback)
{
    return [emptyCallback]()
    {
        if (!emptyCallback) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            ::yandex::maps::runtime::android::callMethod<void>(
                emptyCallback.get(),
                "onEmpty",
                "()V");
        });
    };
}

} // namespace android
} // namespace docs
} // namespace test

namespace test {
namespace docs {
namespace android {

OnSuccess createOnSuccess(::yandex::maps::runtime::android::JniObject lambdaListenerWithTwoMethods)
{
    return [lambdaListenerWithTwoMethods]()
    {
        if (!lambdaListenerWithTwoMethods) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            ::yandex::maps::runtime::android::callMethod<void>(
                lambdaListenerWithTwoMethods.get(),
                "onSuccess",
                "()V");
        });
    };
}

OnError createOnError(::yandex::maps::runtime::android::JniObject lambdaListenerWithTwoMethods)
{
    return [lambdaListenerWithTwoMethods](
        int error)
    {
        if (!lambdaListenerWithTwoMethods) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            ::yandex::maps::runtime::android::callMethod<void>(
                lambdaListenerWithTwoMethods.get(),
                "onError",
                "(I)V",
                ::yandex::maps::runtime::android::toJavaNoLocal(::yandex::maps::runtime::bindings::android::toPlatform(error)));
        });
    };
}

} // namespace android
} // namespace docs
} // namespace test

namespace test {
namespace docs {
namespace android {

OnCallback createOnCallback(::yandex::maps::runtime::android::JniObject callbackWithParam)
{
    return [callbackWithParam](
        int i)
    {
        if (!callbackWithParam) {
            return;
        }

        ::yandex::maps::runtime::verifyUiAndRun([&] {
            ::yandex::maps::runtime::android::callMethod<void>(
                callbackWithParam.get(),
                "onCallback",
                "(I)V",
                ::yandex::maps::runtime::android::toJavaNoLocal(::yandex::maps::runtime::bindings::android::toPlatform(i)));
        });
    };
}

} // namespace android
} // namespace docs
} // namespace test

namespace {

::test::docs::TooInternal* nativeTooInternal(jobject self)
{
    return ::yandex::maps::runtime::android::uniqueGet<::test::docs::TooInternal>(self);
}

} // namespace

namespace {

::test::docs::TooExternal* nativeTooExternal(jobject self)
{
    return ::yandex::maps::runtime::android::uniqueGet<::test::docs::TooExternal>(self);
}

} // namespace

extern "C" {

JNIEXPORT jboolean JNICALL
Java_ru_test_docs_internal_TooInternalBinding_regularMethod__Lru_test_docs_SuchHidden_2Lru_test_docs_VeryPrivate_2(
    JNIEnv* env,
    jobject self,
    jobject muchPrivate,
    jobject soInternal)
{
    BEGIN_NATIVE_FUNCTION;
    REQUIRE(muchPrivate, "Required method parameter \"muchPrivate\" cannot be null");
    REQUIRE(soInternal, "Required method parameter \"soInternal\" cannot be null");

    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(nativeTooInternal(self)->regularMethod(
        ::yandex::maps::runtime::bindings::android::toNative<const ::test::docs::SuchHidden>(muchPrivate),
        ::yandex::maps::runtime::bindings::android::toNative<std::shared_ptr<::test::docs::VeryPrivate>>(soInternal))));

    END_NATIVE_FUNCTION(env);
    return {};
}

JNIEXPORT jboolean JNICALL
Java_ru_test_docs_internal_TooExternalBinding_regularMethod__Lru_test_docs_MuchUnprotected_2Lru_test_docs_SoDeclassified_2(
    JNIEnv* env,
    jobject self,
    jobject structure,
    jobject callback)
{
    BEGIN_NATIVE_FUNCTION;
    REQUIRE(structure, "Required method parameter \"structure\" cannot be null");

    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(nativeTooExternal(self)->regularMethod(
        ::yandex::maps::runtime::bindings::android::toNative<const std::shared_ptr<::test::docs::MuchUnprotected>>(structure),
        test::docs::android::createCantMarkMethodsAsInternalHereYet(callback))));

    END_NATIVE_FUNCTION(env);
    return {};
}

JNIEXPORT void JNICALL
Java_ru_test_docs_internal_TooExternalBinding_hiddenMethod__Lru_test_docs_SuchHidden_2Lru_test_docs_SoSecret_2(
    JNIEnv* env,
    jobject self,
    jobject structure,
    jobject callback)
{
    BEGIN_NATIVE_FUNCTION;
    REQUIRE(structure, "Required method parameter \"structure\" cannot be null");

    nativeTooExternal(self)->hiddenMethod(
        ::yandex::maps::runtime::bindings::android::toNative<const ::test::docs::SuchHidden>(structure),
        test::docs::android::createFirstCallback(callback));

    END_NATIVE_FUNCTION(env);
}

} // extern "C"

extern "C" {

JNIEXPORT jobject JNICALL Java_ru_test_docs_VeryPrivate_init(
    JNIEnv* env,
    jobject /* this */,
    jint regularField,
    jfloat oneMoreRegularField)
{
    BEGIN_NATIVE_FUNCTION

    auto self = std::make_shared<::test::docs::VeryPrivate>();
    self->regularField = ::yandex::maps::runtime::bindings::android::toNative<typename std::remove_reference<decltype(self->regularField)>::type>(regularField);
    self->oneMoreRegularField = ::yandex::maps::runtime::bindings::android::toNative<typename std::remove_reference<decltype(self->oneMoreRegularField)>::type>(oneMoreRegularField);

    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::android::makeSharedObject<::test::docs::VeryPrivate>(self));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jint JNICALL Java_ru_test_docs_VeryPrivate_getRegularField_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = ::yandex::maps::runtime::android::sharedGet<::test::docs::VeryPrivate>(self)->regularField;
    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jfloat JNICALL Java_ru_test_docs_VeryPrivate_getOneMoreRegularField_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = ::yandex::maps::runtime::android::sharedGet<::test::docs::VeryPrivate>(self)->oneMoreRegularField;
    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_VeryPrivate_loadNative(
    JNIEnv* env,
    jclass /* cls */,
    jobject buffer)
{
    BEGIN_NATIVE_FUNCTION

    return ::yandex::maps::runtime::bindings::android::internal::deserializeNative<::test::docs::VeryPrivate>(buffer);

    END_NATIVE_FUNCTION(env);
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_VeryPrivate_saveNative(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    return ::yandex::maps::runtime::bindings::android::internal::serializeNative<::test::docs::VeryPrivate>(self);

    END_NATIVE_FUNCTION(env);
    return {};
}

} // extern "C"

template YANDEX_EXPORT boost::any yandex::maps::runtime::any::internal::BridgedHolder<::test::docs::VeryPrivate>::platformObject() const;


extern "C" {

JNIEXPORT jobject JNICALL Java_ru_test_docs_VeryOpen_init(
    JNIEnv* env,
    jobject /* this */,
    jint regularField,
    jobject hiddenSwitch)
{
    BEGIN_NATIVE_FUNCTION

    auto self = std::make_shared<::test::docs::VeryOpen>();
    self->regularField = ::yandex::maps::runtime::bindings::android::toNative<typename std::remove_reference<decltype(self->regularField)>::type>(regularField);
    self->hiddenSwitch = ::yandex::maps::runtime::bindings::android::toNative<typename std::remove_reference<decltype(self->hiddenSwitch)>::type>(hiddenSwitch);

    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::android::makeSharedObject<::test::docs::VeryOpen>(self));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jint JNICALL Java_ru_test_docs_VeryOpen_getRegularField_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = ::yandex::maps::runtime::android::sharedGet<::test::docs::VeryOpen>(self)->regularField;
    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_VeryOpen_getHiddenSwitch_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = ::yandex::maps::runtime::android::sharedGet<::test::docs::VeryOpen>(self)->hiddenSwitch;
    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_VeryOpen_loadNative(
    JNIEnv* env,
    jclass /* cls */,
    jobject buffer)
{
    BEGIN_NATIVE_FUNCTION

    return ::yandex::maps::runtime::bindings::android::internal::deserializeNative<::test::docs::VeryOpen>(buffer);

    END_NATIVE_FUNCTION(env);
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_VeryOpen_saveNative(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    return ::yandex::maps::runtime::bindings::android::internal::serializeNative<::test::docs::VeryOpen>(self);

    END_NATIVE_FUNCTION(env);
    return {};
}

} // extern "C"

template YANDEX_EXPORT boost::any yandex::maps::runtime::any::internal::BridgedHolder<::test::docs::VeryOpen>::platformObject() const;


extern "C" {

JNIEXPORT jobject JNICALL Java_ru_test_docs_MuchUnprotected_init(
    JNIEnv* env,
    jobject /* this */,
    jfloat regularField,
    jboolean oneMoreRegularField,
    jobject hiddenField)
{
    BEGIN_NATIVE_FUNCTION

    auto self = std::make_shared<::test::docs::MuchUnprotected>();
    self->regularField = ::yandex::maps::runtime::bindings::android::toNative<typename std::remove_reference<decltype(self->regularField)>::type>(regularField);
    self->oneMoreRegularField = ::yandex::maps::runtime::bindings::android::toNative<typename std::remove_reference<decltype(self->oneMoreRegularField)>::type>(oneMoreRegularField);
    self->hiddenField = ::yandex::maps::runtime::bindings::android::toNative<typename std::remove_reference<decltype(self->hiddenField)>::type>(hiddenField);

    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::android::makeSharedObject<::test::docs::MuchUnprotected>(self));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jfloat JNICALL Java_ru_test_docs_MuchUnprotected_getRegularField_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = ::yandex::maps::runtime::android::sharedGet<::test::docs::MuchUnprotected>(self)->regularField;
    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jboolean JNICALL Java_ru_test_docs_MuchUnprotected_getOneMoreRegularField_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = ::yandex::maps::runtime::android::sharedGet<::test::docs::MuchUnprotected>(self)->oneMoreRegularField;
    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_MuchUnprotected_getHiddenField_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = ::yandex::maps::runtime::android::sharedGet<::test::docs::MuchUnprotected>(self)->hiddenField;
    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_MuchUnprotected_loadNative(
    JNIEnv* env,
    jclass /* cls */,
    jobject buffer)
{
    BEGIN_NATIVE_FUNCTION

    return ::yandex::maps::runtime::bindings::android::internal::deserializeNative<::test::docs::MuchUnprotected>(buffer);

    END_NATIVE_FUNCTION(env);
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_MuchUnprotected_saveNative(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    return ::yandex::maps::runtime::bindings::android::internal::serializeNative<::test::docs::MuchUnprotected>(self);

    END_NATIVE_FUNCTION(env);
    return {};
}

} // extern "C"

template YANDEX_EXPORT boost::any yandex::maps::runtime::any::internal::BridgedHolder<::test::docs::MuchUnprotected>::platformObject() const;


extern "C" {

JNIEXPORT jobject JNICALL Java_ru_test_docs_WithInternalEnum_init(
    JNIEnv* env,
    jobject /* this */,
    jobject e)
{
    BEGIN_NATIVE_FUNCTION

    auto self = std::make_shared<::test::docs::WithInternalEnum>();
    self->e = ::yandex::maps::runtime::bindings::android::toNative<typename std::remove_reference<decltype(self->e)>::type>(e);

    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::android::makeSharedObject<::test::docs::WithInternalEnum>(self));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_WithInternalEnum_getE_1_1Native(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    auto& field = ::yandex::maps::runtime::android::sharedGet<::test::docs::WithInternalEnum>(self)->e;
    return ::yandex::maps::runtime::android::toJava(::yandex::maps::runtime::bindings::android::toPlatform(field));

    END_NATIVE_FUNCTION(env)
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_WithInternalEnum_loadNative(
    JNIEnv* env,
    jclass /* cls */,
    jobject buffer)
{
    BEGIN_NATIVE_FUNCTION

    return ::yandex::maps::runtime::bindings::android::internal::deserializeNative<::test::docs::WithInternalEnum>(buffer);

    END_NATIVE_FUNCTION(env);
    return {};
}

JNIEXPORT jobject JNICALL Java_ru_test_docs_WithInternalEnum_saveNative(
    JNIEnv *env,
    jobject self)
{
    BEGIN_NATIVE_FUNCTION

    return ::yandex::maps::runtime::bindings::android::internal::serializeNative<::test::docs::WithInternalEnum>(self);

    END_NATIVE_FUNCTION(env);
    return {};
}

} // extern "C"

template YANDEX_EXPORT boost::any yandex::maps::runtime::any::internal::BridgedHolder<::test::docs::WithInternalEnum>::platformObject() const;


namespace yandex {
namespace maps {
namespace runtime {
namespace bindings {
namespace android {
namespace internal {

namespace {

struct SuchHiddenRegularFieldFieldTrait {
    static constexpr const char* const name = "regularField";
    static constexpr const char* const typeName = "F";
    using type = float;
};

struct SuchHiddenOneMoreRegularFieldFieldTrait {
    static constexpr const char* const name = "oneMoreRegularField";
    static constexpr const char* const typeName = "I";
    using type = int;
};

struct SuchHiddenTwoMoreRegularFieldsFieldTrait {
    static constexpr const char* const name = "twoMoreRegularFields";
    static constexpr const char* const typeName = "F";
    using type = float;
};

} // namespace

YANDEX_EXPORT ::test::docs::SuchHidden ToNative<::test::docs::SuchHidden, jobject, void>::from(
    jobject platformSuchHidden)
{
    return ::test::docs::SuchHidden(
        extractField<::test::docs::SuchHidden, SuchHiddenRegularFieldFieldTrait>(platformSuchHidden),
        extractField<::test::docs::SuchHidden, SuchHiddenOneMoreRegularFieldFieldTrait>(platformSuchHidden),
        extractField<::test::docs::SuchHidden, SuchHiddenTwoMoreRegularFieldsFieldTrait>(platformSuchHidden));
}

YANDEX_EXPORT JniObject ToPlatform<::test::docs::SuchHidden>::from(
    const ::test::docs::SuchHidden& nativeSuchHidden)
{
    static const auto CLASS = runtime::android::findClass(runtime::bindings::BindingTraits<::test::docs::SuchHidden>::javaUndecoratedName);
    static const auto CONSTRUCTOR = runtime::android::constructor(CLASS.get(), "(FIF)V");
    return runtime::android::createObject(
        CLASS.get(),
        CONSTRUCTOR,
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeSuchHidden.regularField())),
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeSuchHidden.oneMoreRegularField())),
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeSuchHidden.twoMoreRegularFields()))
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

struct SuchOptionsInterfaceFieldFieldTrait {
    static constexpr const char* const name = "interfaceField";
    static constexpr const char* const typeName = "Lru/test/docs/TooInternal;";
    using type = std::unique_ptr<::test::docs::TooInternal>;
};

} // namespace

YANDEX_EXPORT ::test::docs::SuchOptions ToNative<::test::docs::SuchOptions, jobject, void>::from(
    jobject platformSuchOptions)
{
    return ::test::docs::SuchOptions(
        extractField<::test::docs::SuchOptions, SuchOptionsInterfaceFieldFieldTrait>(platformSuchOptions));
}

YANDEX_EXPORT JniObject ToPlatform<::test::docs::SuchOptions>::from(
    const ::test::docs::SuchOptions& nativeSuchOptions)
{
    static const auto CLASS = runtime::android::findClass(runtime::bindings::BindingTraits<::test::docs::SuchOptions>::javaUndecoratedName);
    static const auto CONSTRUCTOR = runtime::android::constructor(CLASS.get(), "(Lru/test/docs/TooInternal;)V");
    return runtime::android::createObject(
        CLASS.get(),
        CONSTRUCTOR,
        runtime::android::toJavaNoLocal(runtime::bindings::android::toPlatform(nativeSuchOptions.interfaceField()))
    );
}

} // namespace internal
} // namespace android
} // namespace bindings
} // namespace runtime
} // namespace maps
} // namespace yandex
