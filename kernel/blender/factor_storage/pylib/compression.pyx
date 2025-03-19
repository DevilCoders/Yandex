from libcpp cimport bool
from six import ensure_binary

from util.generic.string cimport TString

cdef extern from "kernel/blender/factor_storage/serialization.h" namespace "NBlender::NProtobufFactors" nogil:
    cdef void CompressString(TString& out, const TString& s, bool base64encode) except +
    cdef void DecompressString(TString& out, const TString& s, bool base64decode) except +


def compress_string(input_str, base64_encode):
    cdef TString result
    CompressString(result, ensure_binary(input_str), base64_encode)
    return result


def decompress_string(input_str, base64_decode):
    cdef TString result
    DecompressString(result, ensure_binary(input_str), base64_decode)
    return result
