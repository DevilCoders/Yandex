__all__ = [
    'ROVER',
]

from copy import deepcopy
from typing import List, Callable, Dict, Optional
from enum import Enum, unique

import attr
import numpy as np
import pandas as pd
from tqdm.auto import tqdm

from ..annotations import TEXT_DATA, TASKS_TEXTS, manage_docstring, Annotation
from ..base import BaseTextsAggregator


@unique
class AlignmentAction(Enum):
    DELETION = 'DELETION'
    SUBSTITUTION = 'SUBSTITUTION'
    INSERTION = 'INSERTION'
    CORRECT = 'CORRECT'


@attr.s
class AlignmentEdge:
    value: str = attr.ib()
    sources_count: Optional[int] = attr.ib()


@attr.s
@manage_docstring
class ROVER(BaseTextsAggregator):
    """
    Recognizer Output Voting Error Reduction (ROVER).

    This method uses dynamic programming to align sequences. Next, aligned sequences are used
    to construct the Word Transition Network (WTN):
    ![ROVER WTN scheme](https://tlk.s3.yandex.net/crowd-kit/docs/rover.png)
    Finally, the aggregated sequence is the result of majority voting on each edge of the WTN.

    J. G. Fiscus,
    "A post-processing system to yield reduced word error rates: Recognizer Output Voting Error Reduction (ROVER),"
    *1997 IEEE Workshop on Automatic Speech Recognition and Understanding Proceedings*, 1997, pp. 347-354.
    <https://doi.org/10.1109/ASRU.1997.659110>

    Args:
        tokenizer: A callable that takes a string and returns a list of tokens.
        detokenizer: A callable that takes a list of tokens and returns a string.
        silent: If false, show a progress bar.

    Examples:
        >>> from crowdkit.aggregation import load_dataset
        >>> from crowdkit.aggregation import ROVER
        >>> df, gt = load_dataset('crowdspeech-test-clean')
        >>> df['text'] = df['text].apply(lambda s: s.lower())
        >>> tokenizer = lambda s: s.split(' ')
        >>> detokenizer = lambda tokens: ' '.join(tokens)
        >>> result = ROVER(tokenizer, detokenizer).fit_predict(df)
    """

    tokenizer: Callable[[str], List[str]] = attr.ib()
    detokenizer: Callable[[List[str]], str] = attr.ib()
    silent: bool = attr.ib(default=True)

    # Available after fit
    # texts_

    @manage_docstring
    def fit(self, data: TEXT_DATA) -> Annotation(type='ROVER', title='self'):
        """
        Fits the model. The aggregated results are saved to the `texts_` attribute.
        """

        result = {}
        grouped_tasks = data.groupby('task') if self.silent else tqdm(data.groupby('task'))
        for task, df in grouped_tasks:
            hypotheses = [self.tokenizer(text) for i, text in enumerate(df['text'])]

            edges = self._build_word_transition_network(hypotheses)
            rover_result = self._get_result(edges)

            text = self.detokenizer([value for value in rover_result if value != ''])

            result[task] = text

        result = pd.Series(result, name='text')
        result.index.name = 'task'
        self.texts_ = result
        return self

    @manage_docstring
    def fit_predict(self, data: TEXT_DATA) -> TASKS_TEXTS:
        """
        Fit the model and return the aggregated texts.
        """

        self.fit(data)
        return self.texts_

    def _build_word_transition_network(self, hypotheses) -> List[Dict[str, AlignmentEdge]]:
        edges = [{edge.value: edge} for edge in self._get_edges_for_words(hypotheses[0])]
        for sources_count, hyp in enumerate(hypotheses[1:], start=1):
            edges = self._align(edges, self._get_edges_for_words(hyp), sources_count)
        return edges

    @staticmethod
    def _get_edges_for_words(words):
        return [AlignmentEdge(word, 1) for word in words]

    @staticmethod
    def _align(
        ref_edges_sets: List[Dict[str, AlignmentEdge]],
        hyp_edges: List[AlignmentEdge],
        sources_count: int
    ) -> List[Dict[str, AlignmentEdge]]:
        """Sequence alignment algorithm implementation.

        Aligns a sequence of sets of tokens (edges) with a sequence of tokens using dynamic programming algorithm. Look
        for section 2.1 in <https://doi.org/10.1109/ASRU.1997.659110> for implementation details. Penalty for
        insert/deletion or mismatch is 1.

        Args:
           ref_edges_sets: Sequence of sets formed from previously aligned sequences.
           hyp_edges: Tokens from hypothesis (currently aligned) sequence.
           sources_count: Number of previously aligned sequences.
        """

        distance = np.zeros((len(hyp_edges) + 1, len(ref_edges_sets) + 1))
        distance[:, 0] = np.arange(len(hyp_edges) + 1)
        distance[0, :] = np.arange(len(ref_edges_sets) + 1)

        memoization = [[None] * (len(ref_edges_sets) + 1) for _ in range(len(hyp_edges) + 1)]
        for i, hyp_edge in enumerate(hyp_edges, start=1):
            memoization[i][0] = (
                AlignmentAction.INSERTION,
                {'': AlignmentEdge('', sources_count)},
                hyp_edge,
            )
        for i, ref_edges in enumerate(ref_edges_sets, start=1):
            memoization[0][i] = (
                AlignmentAction.DELETION,
                ref_edges,
                AlignmentEdge('', 1),
            )

        # find alignment minimal cost using dynamic programming algorithm
        for i, hyp_edge in enumerate(hyp_edges, start=1):
            hyp_word = hyp_edge and hyp_edge.value
            for j, ref_edges in enumerate(ref_edges_sets, start=1):
                ref_words_set = ref_edges.keys()
                is_hyp_word_in_ref = hyp_word in ref_words_set

                options = []

                if is_hyp_word_in_ref:
                    options.append((
                        distance[i - 1, j - 1],
                        (
                            AlignmentAction.CORRECT,
                            ref_edges,
                            hyp_edge,
                        )
                    ))
                else:
                    options.append((
                        distance[i - 1, j - 1] + 1,
                        (
                            AlignmentAction.SUBSTITUTION,
                            ref_edges,
                            hyp_edge,
                        )
                    ))
                options.append((
                    distance[i, j - 1] + ('' not in ref_edges),
                    (
                        AlignmentAction.DELETION,
                        ref_edges,
                        AlignmentEdge('', 1),
                    )
                ))
                options.append((
                    distance[i - 1, j] + 1,
                    (
                        AlignmentAction.INSERTION,
                        {'': AlignmentEdge('', sources_count)},
                        hyp_edge,
                    )
                ))

                distance[i, j], memoization[i][j] = min(options, key=lambda t: t[0])

        alignment = []
        i = len(hyp_edges)
        j = len(ref_edges_sets)

        # reconstruct answer from dp array
        while i != 0 or j != 0:
            action, ref_edges, hyp_edge = memoization[i][j]
            joined_edges = deepcopy(ref_edges)
            hyp_edge_word = hyp_edge.value
            if hyp_edge_word not in joined_edges:
                joined_edges[hyp_edge_word] = hyp_edge
            else:
                # if word is already in set increment sources count for future score calculation
                joined_edges[hyp_edge_word].sources_count += 1
            alignment.append(joined_edges)
            if action == AlignmentAction.CORRECT or action == AlignmentAction.SUBSTITUTION:
                i -= 1
                j -= 1
            elif action == AlignmentAction.INSERTION:
                i -= 1
            # action == AlignmentAction.DELETION
            else:
                j -= 1

        return alignment[::-1]

    @staticmethod
    def _get_result(edges: List[Dict[str, AlignmentEdge]]) -> List[str]:
        result = []
        for edges_set in edges:
            _, _, value = max((x.sources_count, len(x.value), x.value) for x in edges_set.values())
            result.append(value)
        return result
