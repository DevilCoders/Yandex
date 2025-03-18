# cython: c_string_type=str, c_string_encoding=ascii

from libc.stdlib cimport free

cdef extern from "contrib/libs/llvm12/include/llvm/Demangle/Demangle.h" namespace "llvm":
    char *itaniumDemangle(const char *mangled_name, char *buf, size_t *n, int *status)

    cpdef enum MSDemangleFlags:
        MSDF_None "llvm::MSDemangleFlags::MSDF_None"

    char *microsoftDemangle(
        const char *mangled_name,
        size_t *n_read,
        char *buf,
        size_t *n,
        int *status,
        MSDemangleFlags Flags
    )


def itanium(mangled_name):
    cdef char* buf = NULL
    cdef size_t n = 0
    cdef int status
    try:
        buf = itaniumDemangle(mangled_name, buf, &n, &status)
        return status or buf
    finally:
        free(buf)


def microsoft(mangled_name):
    cdef char* buf = NULL
    cdef size_t n_read = 0
    cdef size_t n = 0
    cdef int status
    try:
        buf = microsoftDemangle(
            mangled_name,
            &n_read,
            buf,
            &n,
            &status,
            MSDemangleFlags.MSDF_None
        )
        return status or buf
    finally:
        free(buf)
