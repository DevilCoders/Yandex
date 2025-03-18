from cython.operator import dereference
from cython.operator import preincrement
from libcpp cimport bool
from libcpp.pair cimport pair
from libcpp.string cimport string
from libcpp.vector cimport vector

from util.generic.string cimport TString
from util.generic.string cimport TStringBuf
from util.generic.vector cimport TVector
from util.stream.output cimport IOutputStream
from util.stream.str cimport TStringOutput, TStringOutputPtr

import cPickle as pickle


ctypedef unsigned int ui32


cdef extern from 'util/stream/input.h' nogil:
    cdef cppclass IInputStream:
        IInputStream()


cdef extern from 'util/stream/mem.h' nogil:
    cdef cppclass TMemoryInput(IInputStream):
        TMemoryInput(const void* buf, size_t len)
        TMemoryInput(const TStringBuf s)


cdef extern from 'library/cpp/infected_masks/infected_masks.h' nogil:
    cdef cppclass TSafeBrowsingMasks:
        ctypedef TVector[pair[TString, ui32]] TMatchesType

        cppclass IInitializer:
            void AddMask(const TStringBuf& mask, const TStringBuf& value)
            void Finalize()

        TSafeBrowsingMasks()
        TSafeBrowsingMasks.IInitializer* GetInitializer()

        bool IsInfectedUrl(const TStringBuf& url, TMatchesType* matches)
        void ReadValues(size_t offset, TVector[TString]& result) const
        void Save(IOutputStream* out_s)
        void Load(IInputStream* in_s)


class Error(Exception):
    """Base class for errors in the module."""


cdef class BaseMatcher:
    cdef TSafeBrowsingMasks* _native

    def __cinit__(self):
        self._native = NULL

    cdef _set_native(self, TSafeBrowsingMasks* native):
        if self._native != NULL:
            del self._native
        self._native = native

    def __dealloc__(self):
        del self._native

    def IsInfectedUrl(self, const TStringBuf& url):
        return self._native.IsInfectedUrl(url, NULL)

    def FindAll(self, const TStringBuf& url):
        cdef TSafeBrowsingMasks.TMatchesType matches
        cdef TVector[TString] values
        result = []

        if self._native == NULL:
            raise Error('Use of uninitialized BaseMatcher')

        if self._native.IsInfectedUrl(url, &matches):
            match_iter = matches.begin()
            while match_iter != matches.end():
                values.clear()
                match = dereference(match_iter)
                self._native.ReadValues(match.second, values);
                if values.empty():
                    values.push_back('')
                result.append((match.first, values))
                preincrement(match_iter)
        return result

    def __reduce__(self):
        cdef TString data
        cdef TStringOutputPtr dataStream = TStringOutputPtr(new TStringOutput(data))
        try:
            self._native.Save(dataStream.Get())
        finally:
            pass
        return (BaseMatcher, (), data)  # string(data.data(), data.size()))

    def __setstate__(self, const TStringBuf& state):
        cdef TSafeBrowsingMasks* native = NULL
        cdef TMemoryInput* dataStream = NULL
        try:
            native = new TSafeBrowsingMasks()
            dataStream = new TMemoryInput(state)
            native.Load(dataStream)
            self._set_native(native)
        except:
            del native
            raise
        finally:
            del dataStream


cdef class BaseMatcherMaker:
    cdef TSafeBrowsingMasks* _matcher
    cdef TSafeBrowsingMasks.IInitializer* _maker

    def __cinit__(self):
        self._matcher = new TSafeBrowsingMasks()
        self._maker = self._matcher.GetInitializer()

    def __dealloc__(self):
        del self._maker
        del self._matcher

    def AddMask(self, const TStringBuf& mask, const TStringBuf& data):
        self.EnsureInitializer()
        self._maker.AddMask(mask, data)

    def GetMatcher(self):
        self.EnsureInitializer()
        self._maker.Finalize()
        del self._maker
        self._maker = NULL
        result = BaseMatcher()
        result._set_native(self._matcher)
        self._matcher = NULL
        return result

    cdef void EnsureInitializer(self):
        if self._maker == NULL:
            raise Error(
                'BaseMatcherMaker may only be used once to build a BaseMatcher'
            )


class Matcher(object):
    def __init__(self, base_matcher):
        self._base_matcher = base_matcher

    def IsInfectedUrl(self, const TStringBuf& url):
        return self._base_matcher.IsInfectedUrl(url)

    def FindAll(self, const TStringBuf& url):
        return [
            (
                mask,
                tuple(
                    pickle.loads(item) if item else None
                    for item in data
                )
            )
            for mask, data in self._base_matcher.FindAll(url)
        ]


class MatcherMaker(object):
    def __init__(self):
        self._base_maker = BaseMatcherMaker()

    def AddMask(self, const TStringBuf& mask, data=None):
        data_str = ''
        if data is not None:
            data_str = pickle.dumps(data, protocol=pickle.HIGHEST_PROTOCOL)
        self._base_maker.AddMask(mask, data_str)

    def GetMatcher(self):
        return Matcher(self._base_maker.GetMatcher())
