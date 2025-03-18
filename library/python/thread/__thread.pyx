from util.generic.string cimport TString, TStringBuf

cdef extern from "util/system/thread.h":
    cdef cppclass TThread:
        @staticmethod
        void SetCurrentThreadName(const char* name)


def set_thread_name(name):
    TThread.SetCurrentThreadName(name)
