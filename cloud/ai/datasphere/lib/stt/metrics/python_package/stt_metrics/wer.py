import json
from typing import Dict, Optional, Tuple

from .resources import ClusterReferences
from .alignment_utils import visualize_raw_alignment, align_hypo_and_ref_for_words_wrapper
from .common import MetricData, Metric, evaluate_metric


class WERData(MetricData):
    errors_count: int
    ref_words_count: int
    hyp_words_count: int
    diff_ref: str
    diff_hyp: str

    def __init__(self,
                 errors_count: int,
                 hyp_words_count: int,
                 ref_words_count: int,
                 diff_hyp: str = '',
                 diff_ref: str = ''):
        """
        Store WER data.

        :param errors_count: int
            Number of word which differ in the hypothesis and in the reference.
        :param ref_words_count: int
            Number of words in the reference.
        :param hyp_words_count: int
            Number of words in the hypothesis.
        :param diff_ref: str
            Reference text with highlighted differences from the hypothesis text.
        :param diff_hyp: str
            Hypothesis text with highlighted differences from the reference text.
        """
        self.errors_count = errors_count
        self.ref_words_count = ref_words_count
        self.hyp_words_count = hyp_words_count
        self.diff_ref = diff_ref
        self.diff_hyp = diff_hyp

    def __eq__(self, other: 'WERData'):
        return (
            self.errors_count == other.errors_count and
            self.ref_words_count == other.ref_words_count and
            self.hyp_words_count == other.hyp_words_count and
            self.diff_ref == other.diff_ref and
            self.diff_hyp == other.diff_hyp
        )

    def __repr__(self):
        return '{\n'                                 \
              f'  errors: {self.errors_count},\n'    \
              f'  ref_wc: {self.ref_words_count},\n' \
              f'  hyp_wc: {self.hyp_words_count},\n' \
              f'  diff_ref: {self.diff_ref}\n'       \
              f'  diff_hyp: {self.diff_hyp},\n'      \
               '}'

    def to_json(self) -> Dict:
        return {
            'errors': self.errors_count,
            'ref_wc': self.ref_words_count,
            'hyp_wc': self.hyp_words_count,
            'diff_ref': self.diff_ref,
            'diff_hyp': self.diff_hyp,
        }

    @staticmethod
    def from_json(fields: Dict) -> 'WERData':
        return WERData(
            errors_count=fields['errors'],
            ref_words_count=fields['ref_wc'],
            hyp_words_count=fields['hyp_wc'],
            diff_ref=fields['diff_ref'],
            diff_hyp=fields['diff_hyp'],
        )


class WER(Metric):
    _cr: ClusterReferences

    def __init__(self, cr: Optional[ClusterReferences] = None):
        """
        Creates an object to evaluate Word Error Rate metric.

        :param cr: ClusterReferences (default None)
            If present, specified clusters of words will be treated equally when evaluating metric.
        """
        super(WER, self).__init__(name='WER')
        if cr:
            self._cr = cr
        else:
            self._cr = ClusterReferences()

    def get_metric_data(self, ref: str, hyp: str, fast: bool = False) -> WERData:
        """
        Computes values, which could be used both to evaluate WER metric and to visualize differences between texts.

        :param ref: str
            Reference text.
        :param hyp: str
            Hypothesis text.
        :param fast: bool
            Use fast implementation (may not work in some cases)

        :return: WERData object, which stores both general text stats (differences count, words count)
            and visualization of differences in texts.
        """
        errors_count, hyp_words_count, ref_words_count, alignment = \
            align_hypo_and_ref_for_words_wrapper(hyp, ref, self._cr, use_spn_edges=not fast)
        diff_hyp, diff_ref = visualize_raw_alignment(alignment)
        return WERData(errors_count, ref_words_count, hyp_words_count, diff_ref, diff_hyp)

    @staticmethod
    def calculate_metric(metric_data: WERData) -> float:
        """
        Evaluates WER metric based on the computed WERData.

        :param metric_data: float
            WERData object, which is returned by get_metric_data function.

        :return: WER value.
        """
        words_count = max(metric_data.hyp_words_count, metric_data.ref_words_count, 1)
        return min(metric_data.errors_count / words_count, 1.0)


def evaluate_wer(references: Dict[str, str],
                 hypotheses: Dict[str, str],
                 cluster_references: Optional[ClusterReferences] = None) -> Tuple[float, Dict[str, Dict]]:
    """
    Evaluates WER for several reference-hypothesis text pairs.
    Corresponding to each other reference and hypothesis texts should be provided with the unique id.

    :param references: Dict[str, str]
        an id-text mapping for every reference
    :param hypotheses: Dict[str, str]
        an id-text mapping for every hypothesis
    :param cluster_references: ClusterReferences (default None)
        if present, specified clusters of words will be treated equally when evaluating metric

    :return: mean WER value, as well as WER value and reference-hypothesis text alignment for every pair of texts

    Examples:
    >>> mean_wer, full_stats = evaluate_wer(
    ...     references={"id1": "один два", "id2": "один два"},
    ...     hypotheses={"id1": "один три", "id2": "один один три"},
    ... )
    >>> mean_wer
    0.5833333333333333
    >>> full_stats
    {
      "id1": {
        "metric_value": 0.5,
        "metric_data": {
          "errors": 1,
          "ref_wc": 2,
          "hyp_wc": 2,
          "diff_ref": "один ДВА"
          "diff_hyp": "один ТРИ",
        },
        "reference": "один два",
        "hypothesis": "один три"
      },
      "id2": {
        "metric_value": 0.6666666666666666,
        "metric_data": {
          "errors": 2,
          "ref_wc": 2,
          "hyp_wc": 3,
          "diff_ref": "один ДВА  ***"
          "diff_hyp": "один ОДИН три",
        },
        "reference": "один два",
        "hypothesis": "один один три"
      }
    }
    """
    wer_calcer = WER(cr=cluster_references)
    return evaluate_metric(references, hypotheses, wer_calcer)
