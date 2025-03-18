from util.generic.string cimport TString, TStringBuf
import sys
from library.python import json

cdef extern from "library/python/spack/wrapper.h" nogil:
    TString ConvertJsonSensorsToSpackV1(TStringBuf jsonData) except +
    TString ConvertSpackV1SensorsToJson(TStringBuf jsonData) except +


def from_json (data):
    cdef TStringBuf data_buf = data
    cdef TString result
    with nogil:
        result = ConvertJsonSensorsToSpackV1(data_buf)

    return result


def to_json (data):
    cdef TStringBuf data_buf = data
    cdef TString result
    with nogil:
        result = ConvertSpackV1SensorsToJson(data_buf)

    return result


def dumps(data):
    json_data = json.dumps(data)
    if sys.version_info[0] == 3:
        json_data = json_data.encode()
    return from_json(json_data)


def loads(data):
    json_data = to_json(data)
    if sys.version_info[0] == 3:
        json_data = json_data.decode()
    return json.loads(json_data)
