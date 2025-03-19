from libc.stddef cimport size_t

from six import text_type

from kernel.facts.dynamic_list_replacer.dynamic_list_replacer cimport (
    TMatchResult,
    FindLongestCommonSlice,
    MatchFactTextWithListCandidate,
    ConvertListDataToRichFact,
    NormalizeTextForIndexer,
    DynamicListReplacerHashVector,
    )


cdef class MatchResult:

    cdef public:
        size_t list_candidate_score
        size_t number_of_header_elements_to_skip

    cdef init(self, TMatchResult& c_matchResult):
        self.list_candidate_score = c_matchResult.ListCandidateScore
        self.number_of_header_elements_to_skip = c_matchResult.NumberOfHeaderElementsToSkip


def find_longest_common_slice(a_hash_vector, b_hash_vector, a_to_common, b_to_common, min_allowed_common_slice_size = 1):
    return FindLongestCommonSlice(a_hash_vector, b_hash_vector, a_to_common, b_to_common, min_allowed_common_slice_size)


def match_fact_text_with_list_candidate(fact_text_hash, list_candidate_index_json):
    match_result = MatchResult()
    match_result.init(MatchFactTextWithListCandidate(
        fact_text_hash,
        list_candidate_index_json.encode('utf-8') if isinstance(list_candidate_index_json, text_type) else list_candidate_index_json,
        ))
    return match_result


def convert_list_data_to_rich_fact(serp_data_json, list_data_json, number_of_header_elements_to_skip):
    serp_data_in_json = u'{}' if not serp_data_json else serp_data_json
    need_convert = isinstance(serp_data_in_json, text_type)
    serp_data_out_json = ConvertListDataToRichFact(
        serp_data_in_json.encode('utf-8') if need_convert else serp_data_in_json,
        list_data_json.encode('utf-8') if isinstance(list_data_json, text_type) else list_data_json,
        number_of_header_elements_to_skip,
        )
    return serp_data_out_json.decode('utf-8') if need_convert else serp_data_out_json


def normalize_text_for_indexer(text):
    need_convert = isinstance(text, text_type)
    norm_text = NormalizeTextForIndexer(
        text.encode('utf-8') if need_convert else text,
        )
    return norm_text.decode('utf-8') if need_convert else norm_text


def dynamic_list_replacer_hash_vector(text):
    return DynamicListReplacerHashVector(
        text.encode('utf-8') if isinstance(text, text_type) else text,
        )
