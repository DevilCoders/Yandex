# distutils: language=c++
# -*- coding: utf-8 -*-
import collections
import numpy as np
from libcpp cimport bool
from libcpp.vector cimport vector
from libcpp.pair cimport pair
from libcpp.map cimport map
cimport cython
from cython.operator cimport dereference

cdef struct Vertex:
    int to
    int token_id
    int token_hash

DEF EMPTY_STR_ID = 0
DEF SPN = '<spn>'
DEF SPN_ID = 1


cdef class StringAsGraph:
    """
    Class representing string as graph, e.g.
       I      love     apples
    o ----- o ------ o ------- o
            \\      //
            \\____//
             looove

    every edge represents a word, some edges might be doubled by cluster references
    every cluster reference produces DISTINCT edge!
    """
    cdef object crs, token_to_id, id_to_token
    cdef bool _use_spn_edges
    cdef public vector[vector[Vertex]] edges  # we access this in tests, that's why `public`

    def __init__(self, string, cluster_references, use_spn_edges=False):
        """
        :param string: string to represent as graph
        :param cluster_references: cluster references
        :param use_spn_edges: add SPN edge which can take any number of words
        """
        self.crs = cluster_references
        self.token_to_id = {'': EMPTY_STR_ID, SPN: SPN_ID}
        self.id_to_token = {EMPTY_STR_ID: '', SPN_ID: SPN}
        self._use_spn_edges = use_spn_edges
        self._build_vertices(string)

    cdef int get_token_id(self, str token):
        if token not in self.token_to_id:
            token_id = len(self.token_to_id)
            self.token_to_id[token] = token_id
            self.id_to_token[token_id] = token
        else:
            token_id = self.token_to_id[token]
        return token_id

    cdef void _build_cluster_references_edges(self, list tokens):
        # iterate over all substrings and add edges from cluster references
        cdef int i, j
        cdef int new_vertex_index
        cdef int from_vertex

        for i in range(len(tokens)):
            for j in range(i, len(tokens)):
                possible_crs = self.crs.get(tuple(tokens[i:(j + 1)]), None)
                if not possible_crs:
                    continue
                # firstly we start from i
                for cluster_reference in possible_crs:
                    from_vertex = i
                    for index, new_vertex in enumerate(cluster_reference):
                        token_id = self.get_token_id(new_vertex)

                        if index != len(cluster_reference) - 1:
                            # we need to add new vertex
                            new_vertex_index = self.edges.size()
                            self.edges.push_back([])
                            self.edges[from_vertex].push_back(Vertex(new_vertex_index, token_id, hash(new_vertex)))
                            from_vertex = new_vertex_index
                        else:
                            # close that branch with edge to j
                            new_vertex_index = j + 1
                            self.edges[from_vertex].push_back(Vertex(new_vertex_index, token_id, hash(new_vertex)))
                            from_vertex = j + 1

    cdef void _add_spn_edges(self, list tokens):
        cdef int i, j

        for i in range(len(tokens) + 1):
            for j in range(i, len(tokens) + 1):
                self.edges[i].push_back(Vertex(j, SPN_ID, hash(SPN)))


    cdef void _build_vertices(self, str string):
        tokens = string.strip().lower().split()

        cdef vector[Vertex] filler
        self.edges = vector[vector[Vertex]](len(tokens) + 1, filler)

        cdef int i
        # add tokens
        for i in range(len(tokens)):
            token_id = self.get_token_id(tokens[i])
            self.edges[i].push_back(Vertex(i + 1, token_id, hash(tokens[i])))

        if self.crs:
            self._build_cluster_references_edges(tokens)
        if self._use_spn_edges:
            self._add_spn_edges(tokens)

        # add selfloops
        for i in range(<int>self.edges.size()):
            self.edges[i].push_back(Vertex(i, EMPTY_STR_ID, hash('')))
        self._renumber_vertices()

    cdef void _dfs(self, int current_vertex, vector[bool] &used, vector[int] &sorted_vertices_holder):
        used[current_vertex] = True

        cdef Vertex vertex
        cdef int vertex_to

        for vertex in self.edges[current_vertex]:
            vertex_to = vertex.to
            if not used[vertex_to]:
                self._dfs(vertex_to, used, sorted_vertices_holder)
        sorted_vertices_holder.push_back(current_vertex)

    cdef void _renumber_vertices(self):
        # renumber vertices using topological sort
        cdef vector[bool] used = vector[bool](self.edges.size(), False)
        cdef vector[int] sorted_vertices_holder
        cdef int i

        for i in range(<int>self.edges.size()):
            if not used[i]:
                self._dfs(i, used, sorted_vertices_holder)

        # revert it
        cdef map[int, int] new_order
        for i in range(sorted_vertices_holder.size() - 1, -1, -1):
            new_order.insert(pair[int, int](sorted_vertices_holder[i], sorted_vertices_holder.size() - 1 - i))

        cdef vector[Vertex] edges
        cdef vector[vector[Vertex]] new_edges = vector[vector[Vertex]](self.edges.size(), edges)
        cdef int indx, vertex_val
        cdef Vertex v

        for indx in range(sorted_vertices_holder.size() - 1, -1, -1):
            vertex_val = sorted_vertices_holder[indx]
            edges.clear()
            for v in self.edges[vertex_val]:
                edges.push_back(Vertex(new_order[v.to], v.token_id, v.token_hash))
            new_edges[sorted_vertices_holder.size() - 1 - indx] = edges
        self.edges = new_edges

    def __len__(self):
        return self.edges.size()

    # this could look better with reference, however there is no easy way of doing it in cython,
    # so we return a pointer here instead
    cdef vector[Vertex] *getitem(self, int indx):
        return &(self.edges[indx])

    cpdef str get_edge_value(self, Vertex edge):
        return self.id_to_token[edge.token_id]


@cython.boundscheck(False)
@cython.wraparound(False)
cdef extern object align_hypo_and_ref(StringAsGraph hypothesis, StringAsGraph reference):
    """
    Main algorithm to calculate alignment between hypo and reference

    Dynamic programming over two graphs is finding best alignment over those graphs
    :param hypothesis: hypo as graph
    :param reference: reference as graph
    :return: #mistakes, #tokens_in_hypothesis, #tokens_in_reference, alignment_result
    """
    cdef int hypo_length = len(hypothesis)
    cdef int reference_length = len(reference)

    distance = np.full((hypo_length, reference_length), 100000, dtype=np.int32)
    cdef int[:, :] distance_view = distance
    distance_view[0][0] = 0

    cdef MemoizationItem filler
    filler.hypo_from = -1
    filler.ref_from = -1
    filler.alignment_result = CAlignmentResult(ActionType.A, EMPTY_STR_ID, EMPTY_STR_ID)
    cdef vector[vector[MemoizationItem]] memoization = vector[vector[MemoizationItem]](
        hypo_length,
        vector[MemoizationItem](reference_length, filler)
    )

    cdef int i, j, hypo_arc_idx, ref_arc_idx
    cdef Vertex hypo_arc, ref_arc
    cdef vector[Vertex] *hypo_edges
    cdef vector[Vertex] *ref_edges
    cdef bint not_equal

    for i in range(hypo_length):
        for j in range(reference_length):

            # for each arc from ith vertex
            hypo_edges = hypothesis.getitem(i)
            for hypo_arc in dereference(hypo_edges):
                # for each arc from jth vertex
                ref_edges = reference.getitem(j)
                for ref_arc in dereference(ref_edges):
                    # if both of them is EPS we
                    if hypo_arc.token_id == EMPTY_STR_ID and ref_arc.token_id == EMPTY_STR_ID:
                        continue
                    # update DP
                    not_equal = hypo_arc.token_hash != ref_arc.token_hash
                    if hypo_arc.token_id == SPN_ID and not_equal:
                        continue
                    if distance_view[hypo_arc.to][ref_arc.to] >= distance_view[i][j] + not_equal:
                        # update distance
                        memoization[hypo_arc.to][ref_arc.to] = _get_alignment_result(i, j,
                                                                                     hypo_arc,
                                                                                     ref_arc,
                                                                                     not_equal)
                        distance_view[hypo_arc.to][ref_arc.to] = distance_view[i][j] + not_equal

    cdef int number_of_mistakes = int(distance_view[hypo_length - 1][reference_length - 1])
    alignment_result, aligned_hypothesis_length, aligned_reference_length = unroll_memoization(memoization,
                                                                                               hypo_length,
                                                                                               reference_length)
    alignment_result = _convert_to_alignment_result(alignment_result, hypothesis, reference)

    return number_of_mistakes, aligned_hypothesis_length, aligned_reference_length, alignment_result


cdef _convert_to_alignment_result(list calignment_results, StringAsGraph hypothesis, StringAsGraph reference):
    alignment_results = []
    cdef CAlignmentResult calignment_result
    for calignment_result in calignment_results:
        alignment_results.append(
            AlignmentResult(
                action=_action_type_to_str(calignment_result.action),
                hypothesis_word=hypothesis.id_to_token[calignment_result.hypothesis_token_id],
                reference_word=reference.id_to_token[calignment_result.reference_token_id]
            )
        )

    return alignment_results


cdef str _action_type_to_str(ActionType action_type):
    if action_type == ActionType.A:
        return 'A'
    elif action_type == ActionType.C:
        return 'C'
    elif action_type == ActionType.D:
        return 'D'
    elif action_type == ActionType.I:
        return 'I'
    elif action_type == ActionType.S:
        return 'S'

cdef enum ActionType:
    A, C, D, I, S

cdef struct CAlignmentResult:
    ActionType action
    int hypothesis_token_id
    int reference_token_id

cdef struct MemoizationItem:
    int hypo_from
    int ref_from
    CAlignmentResult alignment_result

AlignmentResult = collections.namedtuple('AlignmentResult', 'action hypothesis_word reference_word')


cdef ActionType _determine_action(int hypo_from, int ref_from, int hypo_to, int ref_to, bint not_equal):
    if ref_to == ref_from and hypo_to != hypo_from and not_equal:
        return ActionType.I
    if ref_to != ref_from and hypo_to == hypo_from and not_equal:
        return ActionType.D
    if not_equal:
        return ActionType.S
    return ActionType.C


cdef MemoizationItem _get_alignment_result(int hypo_from, int ref_from, Vertex hypo_edge, Vertex ref_edge, bint not_equal):
    cdef MemoizationItem result
    result.hypo_from = hypo_from
    result.ref_from = ref_from
    result.alignment_result = CAlignmentResult(
        action=_determine_action(
            hypo_from,
            ref_from,
            hypo_edge.to,
            ref_edge.to,
            not_equal
        ),
        hypothesis_token_id=hypo_edge.token_id,
        reference_token_id=ref_edge.token_id
    )

    return result


cdef bool _has_path_ended(MemoizationItem item):
    return not (item.hypo_from >= 0 and item.ref_from >= 0)


cdef unroll_memoization(vector[vector[MemoizationItem]] memoization, int hypothesis_length, int reference_length):
    """
    Unroll dynamic programming result

    :param memoization: array with dp results
    :param hypothesis_length: length of hypos (# of vertices)
    :param reference_length: length of references (# of vertices)
    :return: list of AlignmentResult, length of real hypothesis and length of real reference
             (it may vary depending on which cluster references were applied)
    """
    cdef int i = hypothesis_length - 1
    cdef int j = reference_length - 1
    cdef list path = []
    cdef int real_hypothesis_length = 0
    cdef int real_reference_length = 0
    cdef CAlignmentResult current_result

    while not _has_path_ended(memoization[i][j]):
        current_result = memoization[i][j].alignment_result
        path.append(current_result)
        i, j = memoization[i][j].hypo_from, memoization[i][j].ref_from

        if current_result.action in [ActionType.S, ActionType.C, ActionType.I]:
            real_hypothesis_length += 1
        if current_result.action in [ActionType.S, ActionType.C, ActionType.D]:
            real_reference_length += 1

    return path[::-1], int(real_hypothesis_length), int(real_reference_length)


def visualize_raw_alignment(alignment_result):
    """
    Basic formatter function for alignment result
    :param alignment_result: array of AlignmentResult
    :return: two strings, hypo and ref formatted alignment results
    """
    ref_words = []
    hyp_words = []
    for alignment in alignment_result:
        hyp_word = alignment.hypothesis_word
        ref_word = alignment.reference_word
        if alignment.action == 'D':
            hyp_word = '*' * len(ref_word)
        elif alignment.action == 'I':
            ref_word = '*' * len(hyp_word)
        elif alignment.action == 'S':
            hyp_word = hyp_word.upper()
            ref_word = ref_word.upper()
            diff = len(hyp_word) - len(ref_word)
            if diff < 0:
                hyp_word += ' ' * abs(diff)
            elif diff > 0:
                ref_word += ' ' * abs(diff)

        hyp_words.append(hyp_word)
        ref_words.append(ref_word)
    return ' '.join(hyp_words), ' '.join(ref_words)


def align_hypo_and_ref_for_words_wrapper(hypothesis, reference, cluster_references=None, use_spn_edges=True):
    """
    Wrapper function to execute aligning algorithm across words in hypothesis and reference,
    first they are represented as graphs, then alignment is applied
    :param hypothesis: string, hypo.
    :param reference: string, reference of this hypo.
    :param cluster_references: Cluster References object
    :param use_spn_edges: some black magic
    :return: #mistakes, #tokens_in_hypothesis, #tokens_in_reference, alignment_result
    """
    hypothesis_as_graph = StringAsGraph(hypothesis, cluster_references, use_spn_edges=use_spn_edges)
    reference_as_graph = StringAsGraph(reference, cluster_references, use_spn_edges=False)
    return align_hypo_and_ref(hypothesis_as_graph, reference_as_graph)


cdef class StringAsCharGraph(StringAsGraph):
    """
    Class representing string as graph.
    Every edge represents a character, some edges might be doubled by cluster references.
    Every cluster reference produces DISTINCT branch of edges!
    """

    cdef bool use_spaces

    def __init__(self, string, cluster_references, use_spaces=False):
        """
        :param string: string to represent as graph
        :param cluster_references: cluster references
        :param use_spaces: flag specifying if we should count spaces as characters in sentences or not
        """
        self.use_spaces = use_spaces
        super().__init__(string, cluster_references)

    cdef void _build_vertices(self, str string):
        # words are for cluster references
        words = string.strip().lower().split()

        if self.use_spaces:
            chars = list(string.strip().lower())
        else:
            chars = list(string.strip().lower().replace(' ', ''))

        cdef vector[Vertex] filler
        self.edges = vector[vector[Vertex]](len(chars) + 1, filler)

        cdef int i
        # add characters
        for i in range(len(chars)):
            char_id = self.get_token_id(chars[i])
            self.edges[i].push_back(Vertex(i + 1, char_id, hash(chars[i])))

        if self.crs:
            self._build_cluster_references_edges(words)

        # add selfloops
        for i in range(<int>self.edges.size()):
            self.edges[i].push_back(Vertex(i, EMPTY_STR_ID, hash('')))
        self._renumber_vertices()

    cdef void _build_cluster_references_edges(self, list words):
        cdef int i, j
        cdef int from_vertex
        cdef int new_vertex_index

        # iterate over all substrings and add edges from cluster references
        for i in range(len(words)):
            for j in range(i, len(words)):
                possible_crs = self.crs.get(tuple(words[i:(j + 1)]), None)
                if not possible_crs:
                    continue
                # firstly we start from i
                for cluster_reference in possible_crs:
                    from_vertex = sum([len(w) for w in words[:i]])  # number of characters for every word before edge i
                    if self.use_spaces:
                        from_vertex += i

                    for word_index, word in enumerate(cluster_reference):
                        if word_index != len(cluster_reference) - 1:
                            # we need to add new vertex
                            for character in word:
                                char_id = self.get_token_id(character)

                                new_vertex_index = self.edges.size()
                                self.edges.push_back([])
                                self.edges[from_vertex].push_back(Vertex(new_vertex_index, char_id, hash(character)))
                                from_vertex = new_vertex_index
                        else:
                            # close that branch with edge to vertex after word j
                            for char_index, character in enumerate(word):
                                char_id = self.get_token_id(character)

                                if char_index != len(word) - 1:
                                    new_vertex_index = self.edges.size()
                                    self.edges.push_back([])
                                else:
                                    new_vertex_index = sum([len(w) for w in words[: j+1]])
                                    if self.use_spaces:
                                        new_vertex_index += (j + 1) - 1

                                self.edges[from_vertex].push_back(Vertex(new_vertex_index, char_id, hash(character)))
                                from_vertex = new_vertex_index


def align_hypo_and_ref_for_chars_wrapper(hypothesis, reference, cluster_references=None, use_spaces=False):
    """
    Wrapper function to execute aligning algorithm across characters in hypothesis and reference,
    first they are represented as graphs, then alignment is applied
    :param hypothesis: string, hypo.
    :param reference: string, reference of this hypo.
    :param cluster_references: Cluster References object
    :param use_spaces: flag specifying if spaces should be considered in alignment algorithm
    :return: #mistakes, #tokens_in_reference, alignment_result
    """
    hypothesis_as_graph = StringAsCharGraph(hypothesis, cluster_references, use_spaces)
    reference_as_graph = StringAsCharGraph(reference, cluster_references, use_spaces)
    return align_hypo_and_ref(hypothesis_as_graph, reference_as_graph)


def calculate_ser_wrapper(hypothesis, reference, wer_engine, cluster_references=None):
    """
    Wrapper function to calculate SER using wer engine
    :param hypothesis:
    :param reference:
    :param wer_engine: WER Engine, calculates triple
                       (#mistakes, #tokens_in_reference, alignment_result)
                       __call__ function must be defined there.
    :param cluster_references: ClusterReferences object
    :return: SER for current pair of (hypothesis, reference)
    """
    incorrect, _, _ = wer_engine(hypothesis, reference, cluster_references)
    return int(incorrect > 0)


def get_spn_id():
    return SPN_ID
