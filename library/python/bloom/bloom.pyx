from util.generic.ptr cimport THolder
from util.stream.output cimport IOutputStream
from util.stream.str cimport TStringOutput, TStringOutputPtr
from util.generic.string cimport TString
from libcpp cimport bool


cdef extern from "util/stream/input.h":
    cdef cppclass IInputStream  nogil:
        IInputStream()

cdef extern from "util/stream/str.h":
    cdef cppclass TStringInput(IInputStream) nogil:
        TStringInput(const TString& s)

cdef extern from "library/cpp/bloom_filter/bloomfilter.h":
    cdef cppclass TBloomFilter nogil:
        TBloomFilter() except +
        TBloomFilter(size_t elemcount, float error) except +

        bool Has(const TString& val) const
        void Add(const TString& val);

        float EstimateItemNumber() const;
        size_t GetBitCount() const;
        size_t GetHashCount() const;

        void Save(IOutputStream* out) const;
        void Load(IInputStream* inp);

        TBloomFilter& Union(const TBloomFilter& other);

cdef class BloomFilter:
    cdef TBloomFilter _cpp_impl
    cdef float error

    def __init__(self, size_t capacity=100, float error=0.01):
        self._cpp_impl = TBloomFilter(capacity, error)
        self.error = error

    def has(self, bytes data):
        return self._cpp_impl.Has(TString(data, len(data)))

    def __contains__(self, bytes data):
        return self.has(data)

    def add(self, bytes data):
        self._cpp_impl.Add(TString(data, len(data)))

    def __iadd__(self, bytes data):
        self.add(data)
        return self

    def merge(self, BloomFilter other):
        self._cpp_impl.Union(other._cpp_impl)
        return self

    def count(self):
        return self._cpp_impl.EstimateItemNumber()

    def get_bit_count(self):
        return self._cpp_impl.GetBitCount()

    def get_hash_count(self):
        return self._cpp_impl.GetHashCount()

    def __str__(self):
        return 'BloomFilter({} bits, {} hashes, ~{} items)'.format(
            self._cpp_impl.GetBitCount(),
            self._cpp_impl.GetHashCount(),
            self._cpp_impl.EstimateItemNumber()
        )

    def __repr__(self):
        return self.__str__()

def loads(bytes data):
    instance = BloomFilter()
    _load(instance._cpp_impl, data)
    return instance

def dumps(BloomFilter instance):
    return _save(instance._cpp_impl)


cdef _load(TBloomFilter& impl, bytes data):
    cdef THolder[IInputStream] stream = THolder[IInputStream](new TStringInput(TString(data, len(data))))
    impl.Load(stream.Get())

cdef _save(const TBloomFilter& impl):
    cdef TString data
    cdef TStringOutputPtr stream = TStringOutputPtr(new TStringOutput(data))
    impl.Save(stream.Get())
    return data
