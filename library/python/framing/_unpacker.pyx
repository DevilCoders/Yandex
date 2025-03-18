from util.generic.string cimport TString, TStringBuf, npos

cdef extern from "library/cpp/framing/format.h" namespace "NFraming":
    cdef cppclass EFormat:
        pass

cdef extern from "library/cpp/framing/unpacker.h" namespace "NFraming" nogil:
    size_t UnpackFromString(EFormat format, TStringBuf data, TStringBuf& frame, TStringBuf& skipped) except +


cdef _unpack_from_string(int format, TStringBuf data, int pos):
    cdef TStringBuf frame
    cdef TStringBuf skipped
    cdef slice = TStringBuf(data.data() + pos, data.size() - pos)
    p = UnpackFromString(<EFormat> format, slice, frame, skipped)
    if p != npos:
        return p, frame, skipped
    return len(data), None, skipped


def unpack_from_string(format, data, pos):
    return _unpack_from_string(format, data, pos)
