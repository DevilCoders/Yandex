from libcpp cimport bool
from util.generic.string cimport TString
from util.memory.blob cimport TBlob

import six


cdef extern from "library/cpp/infected_masks/masks_comptrie.h" nogil:
    cdef cppclass TResult "TCategMaskComptrie::TResult":
        TResult() except +

    cdef cppclass TCategMaskComptrie:
        TCategMaskComptrie() except +
        void Init(const TBlob& blob) except +
        bool FindByUrl(const TString& url, TResult& values) except +


cdef class CategMaskComptrie:
    cdef TCategMaskComptrie *trie

    def __init__(self):
        raise NotImplementedError(u"Cannot instantiate CategMaskComptrie directly. Use from_* methods instead.")

    @classmethod
    def from_file(cls, path):
        if isinstance(path, six.text_type):
            path = path.encode(u"utf8")
        result = cls.__new__(cls)
        result._load_trie_from_file(path)
        return result

    @classmethod
    def from_bytes(cls, data):
        result = cls.__new__(cls)
        result._load_trie_from_bytes(data)
        return result

    def __contains__(self, item):
        return self._find(item)

    def __dealloc__(self):
        if self.trie is not NULL:
            del self.trie

    cpdef _load_trie_from_file(self, path):
        self._initialize(TBlob.FromFile(path))

    cpdef _load_trie_from_bytes(self, data):
        self._initialize(TBlob.FromString(data))

    cdef _initialize(self, const TBlob& blob):
        self.trie = new TCategMaskComptrie()
        self.trie.Init(blob)

    cpdef _find(self, bytes key):
        cdef TResult result = TResult()
        return self.trie.FindByUrl(key, result)
