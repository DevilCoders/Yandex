from util.generic.hash cimport THashMap
from util.generic.string cimport TString


cdef extern from "library/python/tskv/tskv.h":
    THashMap[TString, TString] TSKVStringToDict(const char*, size_t) except +
    TString DictToTSKVString(THashMap[TString, TString])


def loads(s):
    if isinstance(s, unicode):
        s = s.encode('utf-8')

    try:
        return TSKVStringToDict(s, len(s))
    except Exception as e:
        raise ValueError(str(e))


def dumps(d):
    return DictToTSKVString(d)
