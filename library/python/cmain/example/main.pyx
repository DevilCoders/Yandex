from library.python.cmain.run cimport run_cmain

import sys


cdef extern from "<library/python/cmain/example/main.h>" nogil:
    int RunExampleMain(int argc, char** argv)


def main():
    return run_cmain(RunExampleMain, sys.argv)
