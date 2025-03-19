# coding: utf-8
# cython: wraparound=False

import json
import types

from util.generic.maybe cimport TMaybe
from util.generic.vector cimport TVector
from kernel.ugc.cython.util cimport *
from kernel.ugc.aggregation.python.feedback cimport TFeedbackReader, FeedbackReader
from kernel.ugc.aggregation.proto.review_meta_pb2 import TReviewMeta as ReviewMeta

cimport cython
from libcpp cimport bool

__all__ = [
    'FeedbackReader',
    'FeedbackReaderRecord'
]

class FeedbackReaderRecord:
    def __init__(self, value, time):
        self.value = value
        self.time = time


class AppendOptions:
    def __init__(self, filter_skipped_reviews=None, fix_invalid_ratings=None, add_reviews_time=None, add_photos=None, add_meta=None):
        self.filter_skipped_reviews = filter_skipped_reviews
        self.fix_invalid_ratings = fix_invalid_ratings
        self.add_reviews_time = add_reviews_time
        self.add_photos = add_photos
        self.add_meta = add_meta


cdef inline TFeedbackReader.TAppendOptions make_append_options(options):
    cdef TFeedbackReader.TAppendOptions result
    if options.filter_skipped_reviews is not None:
        result.FilterSkippedReviews(<bool>options.filter_skipped_reviews)
    if options.fix_invalid_ratings is not None:
        result.FixInvalidRatings(<bool>options.fix_invalid_ratings)
    if options.add_reviews_time is not None:
        result.AddReviewsTime(<bool>options.add_reviews_time)
    if options.add_photos is not None:
        result.AddPhotos(<bool>options.add_photos)
    if options.add_meta is not None:
        result.AddMeta(<bool>options.add_meta)
    return result


cdef class FeedbackReader:

    def __cinit__(self, key=None):
        if key:
            self.native.Reset(new TFeedbackReader(TStringBuf(<const char*>key)))
        else:
            self.native.Reset(new TFeedbackReader())

    def append_feedback(self, feedback, options=AppendOptions()):
        cdef TFeedbackReader.TErrorDescr error
        cdef TFeedbackReader.TAppendOptions append_options = make_append_options(options)
        res = self.native.Get().AppendFeedback(pyobj_to_schema(feedback), error, append_options);
        pyerr = json.loads(string_to_pyobj(error.ToJson().ToJson()))
        if pyerr:
            raise Exception("append_feedback failed", pyerr);
        return res

    def load_from_user_data(self, user_data, options=AppendOptions()):
        cdef TFeedbackReader.TErrorDescr error
        cdef TFeedbackReader.TAppendOptions append_options = make_append_options(options)
        res = self.native.Get().LoadFromUserData(pyobj_to_schema(user_data), error, append_options)
        pyerr = json.loads(string_to_pyobj(error.ToJson().ToJson()))
        if pyerr:
            raise Exception("load_from_user_data failed", pyerr);
        return res

    def value(self, table, object, column):
        cdef TMaybe[TStringBuf] res = self.native.Get().Value(
            TStringBuf(<const char*>table),
            TStringBuf(<const char*>object),
            TStringBuf(<const char*>column)
        )
        if res.Empty():
            return None
        else:
            return string_to_pyobj(TString(res.GetRef()))

    def get_object_props(self, table, object):
        cdef const TFeedbackReader.TObjectProps* res = self.native.Get().GetObjectProps(
            TStringBuf(<const char*>table),
            TStringBuf(<const char*>object),
        )
        if res == NULL:
            return None
        obj = {}
        cdef const TFeedbackReader.TObjectProps.value_type* key_value
        cdef TFeedbackReader.TObjectProps.const_iterator iter = res.begin()
        while iter != res.end():
            key_value = &cython.operator.dereference(iter)
            obj[string_to_pyobj(key_value.first)] = FeedbackReaderRecord(
                string_to_pyobj(key_value.second.Value),
                key_value.second.Time.MilliSeconds()
            )
            cython.operator.preincrement(iter)
        return obj

    def get_object_review_time(self, object_name):
        cdef TInstant res = self.native.Get().GetObjectReviewTime(
            TStringBuf(<const char*>object_name)
        )
        return res.MilliSeconds()

    def get_object_rating_time(self, object_name):
        cdef TInstant res = self.native.Get().GetObjectRatingTime(
            TStringBuf(<const char*>object_name)
        )
        return res.MilliSeconds()

    def get_object_review_meta(self, object_name):
        cdef TReviewMeta review_meta_cpp = self.native.Get().GetObjectReviewMeta(
            TStringBuf(<const char*>object_name)
        )
        review_meta = ReviewMeta()
        review_meta.ParseFromString(review_meta_cpp.SerializeAsString())
        return review_meta

    def get_object_photos(self, object_name):
        return self.native.Get().GetObjectPhotos(TString(<const char*>object_name))

    def aggregate(self, keep_empty_tombstone = False, sort_every_table = False):
        cdef TValue res = self.native.Get().Aggregate(<bool>keep_empty_tombstone, <bool>sort_every_table)
        return json.loads(string_to_pyobj(res.ToJson()))

