from libcpp cimport bool, nullptr

from util.generic.array_ref cimport TConstArrayRef
from util.generic.string cimport TString
from util.generic.vector cimport TVector

from util.system.types cimport ui8


cdef extern from "kernel/dssm_applier/utils/utils.h" namespace "NDssmApplier::NUtils":
    cdef cppclass TFloat2UI8Compressor "NDssmApplier::NUtils::TFloat2UI8Compressor":
        @staticmethod
        TVector[ui8] Compress(const TConstArrayRef[float] vect, float* const multiplier)

        @staticmethod
        TVector[float] Decompress(const TConstArrayRef[ui8] vect)


def encode_vector(const TVector[ui8]& v):
    return TString(<const char*>v.data(), v.size())


def compress_embedding(const TVector[float]& row):
    return TFloat2UI8Compressor.Compress(<TConstArrayRef[float]>row, <float*>nullptr)


def encode_embedding(const TVector[float]& row):
    return encode_vector(TFloat2UI8Compressor.Compress(<TConstArrayRef[float]>row, <float*>nullptr))


def decode_embedding(const TString& v):
    return TFloat2UI8Compressor.Decompress(TConstArrayRef[ui8](<const ui8*>v.data(), v.size()))

