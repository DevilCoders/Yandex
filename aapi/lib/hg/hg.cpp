#include <Python.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

#include "hg.h"

namespace {

class THgModule {
private:
    static const char* MODULE_NAME;

public:
    THgModule() {
        Py_Initialize();

        Module = PyImport_ImportModule(MODULE_NAME);
        if (Module == nullptr) {
            ythrow yexception() << "can't import module " << MODULE_NAME;
        }

        IdentifyFunction = PyObject_GetAttrString(Module, "identify");
        if (IdentifyFunction == nullptr || !PyCallable_Check(IdentifyFunction)) {
            Py_DECREF(Module);
            ythrow yexception() << "can't find identify fuction in " << MODULE_NAME;
        }
    }

    TString Identify(const TString& name, const TString& repository) {
        auto* args = PyTuple_New(2);
        if (args == nullptr) {
            ythrow yexception() << "can't create python tuple";
        }

        auto* arg0 = PyString_FromStringAndSize(name.data(), name.size());
        if (arg0 == nullptr) {
            Py_DECREF(args);
            ythrow yexception() << "can't create python string";
        }

        auto* arg1 = PyString_FromStringAndSize(repository.data(), repository.size());
        if (arg1 == nullptr) {
            Py_DECREF(arg0);
            Py_DECREF(args);
            ythrow yexception() << "can't create python string";
        }

        if (PyTuple_SetItem(args, 0, arg0) != 0) {
            Py_DECREF(arg1);
            Py_DECREF(arg0);
            Py_DECREF(args);
            ythrow yexception() << "can't set python tuple element";
        }  // steals arg0 reference

        if (PyTuple_SetItem(args, 1, arg1) != 0) {
            Py_DECREF(arg1);
            Py_DECREF(args);
            ythrow yexception() << "can't set python tuple element";
        }  // steals arg1 reference

        auto* ret = PyObject_CallObject(IdentifyFunction, args);
        Py_DECREF(args);

        if (ret == nullptr) {
            PyErr_Print();
            ythrow yexception() << "hg identify failed";
        }

        const char* str = PyString_AsString(ret);
        if (str == nullptr) {
            PyErr_Print();
            Py_DECREF(ret);
            ythrow yexception() << "hg identify returned non-string object";
        }
        int len = static_cast<int>(PyString_Size(ret));

        TString result(str, len);
        Py_DECREF(ret);

        return result;
    }

private:
    PyObject* Module;
    PyObject* IdentifyFunction;
};

const char* THgModule::MODULE_NAME = "aapi.lib.hg.py.hg";

class THgModuleHolder {
public:
    void Init() {
        Hg = MakeHolder<THgModule>();
    }

    TString Identify(const TString& name, const TString& repository) {
        return Hg->Identify(name, repository);
    }

    static THgModuleHolder* Instance() {
        return SingletonWithPriority<THgModuleHolder, 130000>();
    };

private:
    THolder<THgModule> Hg;
};

}  // namespace

namespace NHg {

void Init() {
    THgModuleHolder::Instance()->Init();
}

TString Identify(const TString& name, const TString& repository) {
    return THgModuleHolder::Instance()->Identify(name, repository);
}

}  // namespace NHg
