#pragma once

#include <Python.h>

namespace NCYson {
    constexpr bool PY3 = PY_MAJOR_VERSION == 3;
    constexpr bool PY2 = PY_MAJOR_VERSION == 2;

    void SetPrettyTypeError(PyObject*, const char*);
    PyObject* ConvertPyStringToPyBytes(PyObject*);
    PyObject* GetCharBufferAndOwner(PyObject*, const char**, size_t*);
    PyObject* ConvertPyStringToPyNativeString(PyObject*);
    PyObject* ConvertPyLongToPyBytes(PyObject*);

    inline PyObject* GetSelf(PyObject* self) {
        Py_INCREF(self);
        return self;
    }

    namespace NPrivate {
        class TPyObjectPtr {
        public:
            void Reset(PyObject*);
            PyObject* GetNew();
            PyObject* GetBorrowed();

            TPyObjectPtr();
            ~TPyObjectPtr();

        private:
            PyObject* Ptr_;
        };
    }
}

#if PY_MAJOR_VERSION >= 3
#define GenericCheckBuffer PyObject_CheckBuffer
#define PyFile_CheckExact(x) 0
#define PyFile_AsFile(x) (FILE*)(PyErr_Format(PyExc_NotImplementedError, "PyFile_AsFile not implemented for Python3"))
#else
#define GenericCheckBuffer PyObject_CheckReadBuffer
#endif
