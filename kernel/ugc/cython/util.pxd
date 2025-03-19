# coding: utf-8
# cython: wraparound=False

from util.generic.string cimport TString, TStringBuf


cdef extern from "util/generic/ptr.h" nogil:
    cdef cppclass TAtomicSharedPtr[T]:
        TAtomicSharedPtr()
        void Reset(T*)
        T* Get()


cdef extern from "util/datetime/base.h" nogil:
    cdef cppclass TInstant:
        unsigned long long MilliSeconds()


cdef extern from "library/cpp/scheme/scheme.h" namespace "NSc" nogil:
    cdef cppclass TValue:
        TValue()
        TString ToJson()
        @staticmethod
        TValue FromJsonThrow(TStringBuf) except +


cdef inline string_to_pyobj(const TString& val) except +:
    return val.c_str().decode("utf-8")


cdef inline TValue pyobj_to_schema(val) except +:
    if isinstance(val, str):
        return TValue.FromJsonThrow(TStringBuf(<const char*>val))
    if isinstance(val, unicode):
        data = val.encode("utf-8")
        return TValue.FromJsonThrow(TStringBuf(<const char*>data))
    raise TypeError("str or unicode expected")

cdef inline TString pyobj_to_string(val) except +:
    if isinstance(val, str):
        return val
    elif isinstance(val, unicode):
        return val.encode("utf8")
    raise TypeError("str or unicode expected")
