import future.utils as fu

from libcpp cimport bool
from util.generic.string cimport TString


cdef extern from "util/generic/guid.h":
    cdef cppclass TGUID:
        TGUID()

        unsigned dw[4]

    cdef void CreateGuid(TGUID* res) except +
    cdef TString GetGuidAsString(const TGUID& g) except +
    cdef bool GetGuid(const TString& s, TGUID& result) except +


def create():
    cdef TGUID g

    CreateGuid(&g)

    return (g.dw[0], g.dw[1], g.dw[2], g.dw[3])


def to_string(gg):
    cdef TGUID g
    cdef TString s

    g.dw[0] = gg[0]
    g.dw[1] = gg[1]
    g.dw[2] = gg[2]
    g.dw[3] = gg[3]

    s = GetGuidAsString(g)

    return fu.bytes_to_native_str(s.c_str()[:s.length()])


def parse(ss):
    byte_str = fu.native_str_to_bytes(ss)

    cdef TString s = TString(<const char*>byte_str, len(byte_str))
    cdef TGUID g

    if not GetGuid(s, g):
        raise Exception('can not parse %s' % ss)

    return (g.dw[0], g.dw[1], g.dw[2], g.dw[3])
