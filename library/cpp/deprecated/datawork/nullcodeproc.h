#pragma once

#include <cstdlib>
//probably this should be in separate library but...
struct TNullCodeProc {
    //accessors are Get Set, but for function - Call
    template <typename T, bool Const, bool Ref>
    void RegisterConvert(const char*) {
    }
    template <typename T>
    void RegisterType(const char*) {
    }
    template <typename T>
    void RegisterBaseType(const char*) {
    }
    template <ui32 v>
    void RegisterStaticConst(const char*) {
    }
    template <typename TAcc, typename T>
    void RegisterStaticFunc(const char*, const T&) {
    }
    template <int n, typename TAcc, typename T>
    void RegisterStaticFuncOther(const char*, const T&) {
    }

    //    template<int n, typename TAcc, typename T> void RegisterFuncOther(const char*, const T&) {}
    //    template<int n, typename TAcc, typename T> void RegisterFuncOtherConst(const char*, const T&) {}

    template <typename TAcc, typename T, typename R>
    void RegisterFuncConst(const char*, R (T::*)() const) {
    }
    template <typename TAcc, typename T, typename R>
    void RegisterFunc(const char*, R (T::*)()) {
    }

    template <typename TAcc>
    void RegisterFuncConst(const char*, void*) {
    }
    template <typename TAcc>
    void RegisterFunc(const char*, void*) {
    }

    template <typename TAcc>
    void RegisterFuncConst(const char*, ...) {
    }
    template <typename TAcc>
    void RegisterFunc(const char*, ...) {
    }

    template <int n, typename TAcc>
    void RegisterFuncOther(const char*, ...) {
    }
    template <int n, typename TAcc>
    void RegisterFuncOtherConst(const char*, ...) {
    }

    template <typename TAcc, typename T>
    void RegisterConst(const char*, const T&) {
    }
    template <typename TAcc, typename T>
    void RegisterOrdinal(const char*, T&) {
    }

    template <int n, typename TAcc>
    void RegisterConstBitfield(const char*, i64) {
    }
    template <int n, typename TAcc>
    void RegisterBitfield(const char*, i64) {
    }

    template <typename TStruct, typename TInfo>
    void RegisterStruct(const char*) {
        TStruct* p = (TStruct*)malloc(sizeof(TStruct));
        Y_VERIFY(p != 0, "can't alloc");
        static_assert((std::is_same<TStruct, typename TInfo::TAboutType>::value), "expect (std::is_same<TStruct, typename TInfo::TAboutType>::value)");
        TInfo::RegisterStructMembers(p, *this);
        free(p);
    }
};
