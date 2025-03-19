import json

from libcpp cimport bool as cbool

from util.folder.path cimport TFsPath
from util.generic.string cimport TString, TStringBuf


cdef extern from "library/cpp/scheme/scheme.h" namespace "NSc" nogil:
    cdef cppclass TJsonOpts:
        TJsonOpts() except +

    cdef cppclass TValue:
        @staticmethod
        TValue FromJsonThrow(TStringBuf, const TJsonOpts&) except +


cdef extern from "kernel/facts/vowpal_wabbit_filters/vowpal_wabbit_filters.h" namespace "NVwFilters" nogil:
    cdef struct TFilterResult:
        const cbool Filtered
        const TString FilteredBy
        const TString Error

    cdef cppclass TVowpalWabbitMultiFilter:
        TVowpalWabbitMultiFilter() except +
        TVowpalWabbitMultiFilter(const TFsPath& fmlsDir, const TFsPath& configPath) except +
        TFilterResult Filter(const TString& query, const TString& answer, const TValue& configPatch) except +


class FilterResult(object):
    def __init__(self, filtered, filtered_by, error):
        self.filtered = filtered
        self.filtered_by = filtered_by
        self.error = error


cdef tzora_response_to_pyobj(TFilterResult tzora_response):
    return FilterResult(
        tzora_response.Filtered,
        tzora_response.FilteredBy,
        tzora_response.Error,
    )


cdef class VowpalWabbitMultiFilter:
    cdef TVowpalWabbitMultiFilter* _native

    def __cinit__(self, TString fmls_dir, TString config_path):
        self._native = new TVowpalWabbitMultiFilter(TFsPath(fmls_dir), TFsPath(config_path))

    def __dealloc__(self):
        del self._native

    def filter(self, query, answer, config_patch=None):
        if config_patch is None:
            config_patch = '{}'
        else:
            config_patch = json.dumps(config_patch)

        if isinstance(query, unicode):
            query = query.encode('utf-8')
        if isinstance(answer, unicode):
            answer = answer.encode('utf-8')

        return tzora_response_to_pyobj(self._native.Filter(query, answer, TValue.FromJsonThrow(TStringBuf(config_patch), TJsonOpts())))
