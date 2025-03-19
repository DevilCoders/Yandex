from util.generic.string cimport TString, TStringBuf
from util.system.types cimport ui32

from libcpp cimport bool


cdef extern from "<kernel/dups/check.h>" namespace "NDups":
    ui32 cGetHashKeyForUrl "NDups::GetHashKeyForUrl" (const TStringBuf& url, bool normalize)

def GetHashKeyForUrl(bytes data):
    cdef TStringBuf url = TStringBuf(data, len(data))
    cdef bool norm = True
    cdef ui32 result = cGetHashKeyForUrl(url, norm)
    return result

