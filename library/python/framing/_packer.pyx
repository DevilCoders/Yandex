from util.generic.string cimport TString, TStringBuf

cdef extern from "library/cpp/framing/format.h" namespace "NFraming":
    cdef cppclass EFormat:
        pass

cdef extern from "library/cpp/framing/packer.h" namespace "NFraming" nogil:
    TString PackToString(EFormat format, TStringBuf text) except +


cdef _pack_to_string(int format, TStringBuf text):
    return PackToString(<EFormat> format, text)


def pack_to_string(format, text):
    return _pack_to_string(format, text)
