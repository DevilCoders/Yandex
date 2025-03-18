from util.system.types cimport ui32, ui64

from util.digest.fnv cimport FnvHash

def hash64(content):
    cdef const char* s = content
    cdef size_t size = len(content)
    return FnvHash[ui64](s, size)


def hash32(content):
    cdef const char* s = content
    cdef size_t size = len(content)
    return FnvHash[ui32](s, size)
