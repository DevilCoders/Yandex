from six import PY3
from util.generic.string cimport TString, TStringBuf


cdef extern from "library/cpp/scheme/scheme.h" namespace "NSc":
    cdef cppclass TJsonOpts:
        TJsonOpts() except +
    cdef cppclass TValue:
        @staticmethod
        TValue FromJsonThrow(TStringBuf, const TJsonOpts&)


cdef extern from "kernel/facts/serp_parser/extract_answer.h" namespace "NFactsSerpParser":
    TString ExtractAnswer(const TValue&)


def extract_answer(serp_data_json):
    if PY3:
        if isinstance(serp_data_json, str):
            serp_data_json = serp_data_json.encode("utf8")
        return ExtractAnswer(TValue.FromJsonThrow(serp_data_json, TJsonOpts())).decode("utf8")
    else:
        if isinstance(serp_data_json, unicode):
            serp_data_json = serp_data_json.encode("utf8")
        return ExtractAnswer(TValue.FromJsonThrow(serp_data_json, TJsonOpts()))
