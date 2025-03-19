from libc.stddef cimport size_t

from util.generic.string cimport TString
from util.generic.vector cimport TVector
from util.system.types cimport ui16


ctypedef TVector[ui16] THashVector

cdef extern from 'kernel/facts/dynamic_list_replacer/dynamic_list_replacer.h' namespace 'NFacts' nogil:

    cppclass TMatchResult:
        size_t ListCandidateScore
        size_t NumberOfHeaderElementsToSkip

    size_t FindLongestCommonSlice(const THashVector& a, const THashVector& b, size_t& aToCommon, size_t& bToCommon, size_t minAllowedCommonSliceSize) except +
    TMatchResult MatchFactTextWithListCandidate(const THashVector& factTextHash, const TString& listCandidateIndexJson) except +
    TString ConvertListDataToRichFact(const TString& serpDataJson, const TString& listDataJson, size_t numberOfHeaderElementsToSkip) except +
    TString NormalizeTextForIndexer(const TString& text) except +
    THashVector DynamicListReplacerHashVector(const TString& text) except +
