from libcpp cimport bool

from util.generic.string cimport TString, TStringBuf


cdef extern from "library/cpp/string_utils/base64/base64.h":
    cdef void Base64Encode(const TStringBuf& src, TString& dst) nogil except +
    cdef void Base64Decode(const TStringBuf& src, TString& dst) nogil except +


def dumps(data):
    cdef TString res
    cdef TStringBuf cdata = TStringBuf(data, len(data))

    if len(data) < 128:
        Base64Encode(cdata, res)
    else:
        with nogil:
            Base64Encode(cdata, res)

    return res.c_str()[:res.length()]


def loads(data):
    cdef TString res
    cdef TStringBuf cdata = TStringBuf(data, len(data))

    if len(data) < 128:
        Base64Decode(cdata, res)
    else:
        with nogil:
            Base64Decode(cdata, res)

    return res.c_str()[:res.length()]
