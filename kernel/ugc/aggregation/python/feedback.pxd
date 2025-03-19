from util.generic.maybe cimport TMaybe
from util.generic.vector cimport TVector
from kernel.ugc.cython.util cimport *

cimport cython
from libcpp cimport bool


cdef extern from "kernel/ugc/aggregation/proto/review_meta.pb.h" namespace "NUgc" nogil:
    cdef cppclass TReviewMeta:
        TReviewMeta()
        TString SerializeAsString()


cdef extern from "kernel/ugc/aggregation/feedback.h" namespace "NUgc" nogil:
    cdef cppclass TFeedbackReader:
        cppclass TErrorDescr:
            TErrorDescr()
            TValue ToJson()

        cppclass TAppendOptions:
            TAppendOptions()
            TAppendOptions& FilterSkippedReviews(bool v)
            TAppendOptions& FixInvalidRatings(bool v)
            TAppendOptions& AddReviewsTime(bool v)
            TAppendOptions& AddPhotos(bool v)
            TAppendOptions& AddMeta(bool v)

        cppclass TRecord:
            TInstant Time
            TString Value

        cppclass TObjectProps:
            cppclass value_type:
                TString first
                TRecord second
            cppclass const_iterator:
                value_type& operator*()
                const_iterator operator++()
                bool operator!=(const_iterator)
            const_iterator begin()
            const_iterator end()

        TFeedbackReader()
        TFeedbackReader(TStringBuf tableKeyName)
        bool AppendFeedback(const TValue& feedback, TErrorDescr& error, const TAppendOptions& options) except +
        size_t LoadFromUserData(const TValue& userData, TErrorDescr& error, const TAppendOptions& options) except +
        TMaybe[TStringBuf] Value(TStringBuf table, TStringBuf object, TStringBuf column) const
        TObjectProps* GetObjectProps(TStringBuf table, TStringBuf object) const
        TValue Aggregate(bool keepEmptyTombstone, bool sortEveryTable) const
        TInstant GetObjectReviewTime(TStringBuf objectName) const
        TInstant GetObjectRatingTime(TStringBuf objectName) const
        TReviewMeta GetObjectReviewMeta(TStringBuf objectName) const
        const TVector[TString]& GetObjectPhotos(TStringBuf objectName) const


cdef class FeedbackReader:
    cdef TAtomicSharedPtr[TFeedbackReader] native
