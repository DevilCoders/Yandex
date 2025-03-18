import sys

from util.generic.string cimport TString
from util.generic.vector cimport TVector

cdef int run_cmain(int (*cmain)(int, char**) nogil, object argv, bint release_gil=True):
    cdef TVector[TString] c_argv_data
    cdef TVector[char*] c_argv

    argv = tuple(argv)
    c_argv_data.reserve(len(argv))
    c_argv.reserve(len(argv))

    for arg in argv:
        # Create copies of strings in case not-so-well-behaving
        # C code modifies values in argv[]
        if sys.version_info[0] == 2:
            c_argv_data.push_back(bytes(arg))
        else:
            c_argv_data.push_back(bytes(arg, encoding="utf-8"))
        c_argv.push_back(<char*>c_argv_data.back().c_str())

    if release_gil:
        with nogil:
            return cmain(c_argv.size(), c_argv.data())
    else:
        return cmain(c_argv.size(), c_argv.data())

