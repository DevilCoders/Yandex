from libcpp.pair cimport pair
from libcpp.memory cimport unique_ptr
from cython.operator cimport dereference
from libc.stdint cimport uint32_t
from util.generic.string cimport TString


cdef extern from "util/stream/buffer.h":
    cdef cppclass TBufferStream:
        TBufferStream() except +


cdef extern from "util/memory/blob.h":
    cdef cppclass TBlob:
        @staticmethod
        TBlob FromStream(TBufferStream)


cdef extern from "library/cpp/on_disk/aho_corasick/writer.h":
    cdef cppclass TDefaultAhoCorasickBuilder:
        TAhoCorasickBuilder() except +
        void AddString(TString, uint32_t) except +
        void SaveToStream(TBufferStream*) except +


cdef extern from "library/cpp/on_disk/aho_corasick/reader.h":
    cdef cppclass TDefaultMappedAhoCorasick:
        cppclass TSearchResult:
            TSearchResult()
            cppclass iterator:
                pair[uint32_t, uint32_t] operator*()
                iterator operator++()
                bint operator==(iterator)
                bint operator!=(iterator)
            iterator begin()
            iterator end()
        TDefaultMappedAhoCorasick(const TBlob&) except +
        TSearchResult AhoSearch(const TString&) except +


cdef class SimpleInMemoryAhoCorasick:
    cdef unique_ptr[TDefaultAhoCorasickBuilder] _c_aho_corasick_builder_ptr
    cdef unique_ptr[TDefaultMappedAhoCorasick] _c_mapped_aho_corasick_ptr
    cdef list _mapping

    def __cinit__(self):
        self._c_aho_corasick_builder_ptr = unique_ptr[TDefaultAhoCorasickBuilder](new TDefaultAhoCorasickBuilder())
        self._mapping = []

    def __init__(self, seq):
        for s in seq:
            self._add_string(s)
        self._load()

    def _ensure_byte_string(self, string):
        if isinstance(string, unicode):
            return string.encode('utf8')
        return string

    def _add_string(self, string):
        byte_string = self._ensure_byte_string(string)
        cdef TString stroka = TString(<char*>byte_string, len(byte_string))
        self._c_aho_corasick_builder_ptr.get().AddString(stroka, len(self._mapping))
        self._mapping.append(string)

    def _load(self):
        cdef unique_ptr[TBufferStream] buffer_stream = unique_ptr[TBufferStream](new TBufferStream())
        self._c_aho_corasick_builder_ptr.get().SaveToStream(buffer_stream.get())
        words_blob = TBlob.FromStream(dereference(buffer_stream.get()))
        self._c_mapped_aho_corasick_ptr = unique_ptr[TDefaultMappedAhoCorasick](new TDefaultMappedAhoCorasick(words_blob))

    def find(self, string):
        byte_string = self._ensure_byte_string(string)
        cdef TString stroka = TString(<char*>byte_string, len(byte_string))
        cdef TDefaultMappedAhoCorasick.TSearchResult search_result = self._c_mapped_aho_corasick_ptr.get().AhoSearch(stroka)
        return [self._mapping[p.second] for p in search_result]
