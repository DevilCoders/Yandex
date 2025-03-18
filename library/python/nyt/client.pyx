# -*- coding: utf-8 -*-
# cython: c_string_type=str, c_string_encoding=utf-8

cdef extern from "<mapreduce/yt/client/init.h>" namespace "NYT" nogil:
    void Initialize(int argc, const char* argv[]);

def initialize(args):
    assert len(args) < 1024

    cdef int argc = 0;
    cdef char* argv[1024]

    for arg in args:
        argv[argc] = arg
        argc += 1

    with nogil:
        Initialize(argc, <const char**>argv)
