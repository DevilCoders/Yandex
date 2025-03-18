# coding: utf-8
from libc.stdint cimport uint32_t
from libcpp.vector cimport vector
from libcpp.string cimport string
import six


cdef extern from "library/python/cookiemy/srcs/setup.h" namespace "cookiemy::CookiemyHandler" nogil:
    ctypedef vector[uint32_t] BlockData;
    ctypedef unsigned char BlockID

cdef extern from "library/python/cookiemy/srcs/setup.h" namespace "cookiemy" nogil:
    cdef cppclass CookiemyHandler:
        CookiemyHandler()

        void parse(const string& cookie) except +RuntimeError
        void insert(BlockID id, const BlockData &data)
        const BlockData& find(BlockID id) const
        vector[BlockID] keys() const
        void erase(BlockID id)
        string toString() const
        void clear()


cdef class Cookiemy:
    cdef CookiemyHandler* _setup

    def __cinit__(self):
        self._setup = new CookiemyHandler()

    def parse(self, cookie):
        self._setup.parse(cookie)

    def insert(self, id, data):
        self._setup.insert(id, data)

    def find(self, id):
        return self._setup.find(id)

    def keys(self):
        return self._setup.keys()

    def erase(self, id):
        return self._setup.erase(id)

    def __str__(self):
        result = self._setup.toString()
        if six.PY3:
            result = result.decode('utf8')
        return result

    def clear(self):
        return self._setup.clear()

    def __dealloc__(self):
        del self._setup


Setup = Cookiemy
