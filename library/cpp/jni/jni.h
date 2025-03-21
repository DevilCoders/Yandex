#pragma once

#include <jni.h>

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NJni {

// All necessary functions are in version 1.4
//
static constexpr int JNI_VERSION = JNI_VERSION_1_4;

// TJniException ////////////////////////////////////////////////////////////////////////////////

class TJniException : public yexception {
public:
    TJniException() = default;
    explicit TJniException(int error);
};

void RethrowExceptionFromJavaToCpp();

// TObjectRef ////////////////////////////////////////////////////////////////////////////////

struct TGlobalRefPolicy {
    static jobject Ref(jobject object, JNIEnv* env) { return env->NewGlobalRef(object); }
    static void Unref(jobject object, JNIEnv* env) { return env->DeleteGlobalRef(object); }
};

struct TLocalRefPolicy {
    static jobject Ref(jobject object, JNIEnv*) { return object; }
    static void Unref(jobject object, JNIEnv* env) { return env->DeleteLocalRef(object); }
};

template <typename TRefPolicy, typename TObject>
class TObjectRef : public TMoveOnly {
public:
    TObjectRef(): Object() {};
    explicit TObjectRef(TObject object);
    TObjectRef(TObjectRef&& rhs) { *this = std::move(rhs); }
    ~TObjectRef();

    TObjectRef& operator= (TObjectRef&& rhs) noexcept;

    operator bool() const { return !!Object; }

public:
    TObject Get() const { return Object; }
    TObject Release();

private:
    TObject Object {};
};

using TGlobalRef = TObjectRef<TGlobalRefPolicy, jobject>;
using TLocalRef = TObjectRef<TLocalRefPolicy, jobject>;

using TLocalClassRef = TObjectRef<TLocalRefPolicy, jclass>;
using TGlobalClassRef = TObjectRef<TGlobalRefPolicy, jclass>;
using TLocalArrayRef = TObjectRef<TLocalRefPolicy, jbyteArray>;
using TLocalStringRef = TObjectRef<TLocalRefPolicy, jstring>;

// TJniEnv //////////////////////////////////////////////////////////////////////////////////////

struct TJniEnv {
private:
    TJniEnv();

public:
    static TJniEnv* Get();

public:
    // Should be used only in pair with JNI_OnLoad/Unload.
    //
    jint Init(JavaVM* jvm);
    void Cleanup(JavaVM* jvm);

    // Thread safe JniEnv.
    //
    JNIEnv* GetJniEnv() const;

    TLocalClassRef FindClass(TStringBuf name) const;
    jmethodID GetMethodID(jclass clazz, TStringBuf name, TStringBuf signature, bool isStatic) const;

    TLocalRef CallStaticObjectMethod(jclass clazz, jmethodID methodId, ...) const;
    TLocalRef CallObjectMethod(jobject object, jmethodID methodId, ...) const;
    jint CallIntMethod(jobject object, jmethodID methodId, ...) const;
    jboolean CallBooleanMethod(jobject object, jmethodID methodId, ...) const;
    void CallVoidMethod(jobject object, jmethodID methodId, ...) const;

    TLocalArrayRef NewByteArray(jsize len) const;
    void SetByteArrayRegion(jbyteArray array, jsize start, jsize len, const char* buf) const;
    void GetByteArrayRegion(jbyteArray array, jsize start, jsize len, char* buf) const;
    jsize GetArrayLength(jarray array) const;
    TLocalStringRef NewStringUTF(TStringBuf str) const;
    const char* GetStringUTFChars(jstring str, jboolean* isCopy) const; // FIXME: leaky without ReleaseUTFChars

private:
    void TryToSetClassLoader();

private:
    struct TResources;
    std::unique_ptr<TResources> Resources;
};

TJniEnv* Env();

}  // namespace NJni
