import typing
from collections import namedtuple

from dataclasses import dataclass
from enum import Enum
from voicetech.asr.tools.asr_analyzer.lib.alignment_utils import visualize_raw_alignment
from voicetech.asr.tools.asr_analyzer.lib.resources import ClusterReference, CachingWEREngine

from ..common import Metric, MetricData


class AlignmentAction(Enum):
    SUBSTITUTION = 'S'
    DELETION = 'D'
    INSERTION = 'I'
    COINCIDENCE = 'C'


# just a wrapper around Alice AlignmentResult named tuple
@dataclass
class AlignmentElement:
    action: AlignmentAction
    ref_word: str
    hyp_word: str

    def to_json(self) -> dict:
        return {
            'action': self.action.value,
            'ref_word': self.ref_word,
            'hyp_word': self.hyp_word,
        }

    @staticmethod
    def from_json(fields: dict) -> 'AlignmentElement':
        return AlignmentElement(
            action=AlignmentAction(fields['action']),
            ref_word=fields['ref_word'],
            hyp_word=fields['hyp_word'],
        )

    @staticmethod
    def from_alice_named_tuple(t: namedtuple) -> 'AlignmentElement':
        return AlignmentElement(
            action=AlignmentAction(t.action),
            ref_word=t.reference_word,
            hyp_word=t.hypothesis_word,
        )


@dataclass
class WERData(MetricData):
    errors_count: int
    ref_words_count: int  # given cluster references
    hyp_words_count: int  # given cluster references
    diff_hyp: str
    diff_ref: str
    alignment: typing.List[AlignmentElement]

    def __init__(
        self,
        errors_count: int,
        ref_words_count: int,
        hyp_words_count: int,
        # TODO: default values below are ugly and needed only for tests, may be split class or refactor tests
        diff_hyp: str = '',
        diff_ref: str = '',
        alignment: typing.List[AlignmentElement] = None,
    ):
        self.errors_count = errors_count
        self.ref_words_count = ref_words_count
        self.hyp_words_count = hyp_words_count
        self.diff_hyp = diff_hyp
        self.diff_ref = diff_ref
        self.alignment = alignment

    def to_json(self) -> dict:
        return {
            'errors': self.errors_count,
            'ref_wc': self.ref_words_count,
            'hyp_wc': self.hyp_words_count,
            'diff_hyp': self.diff_hyp,
            'diff_ref': self.diff_ref,
            'alignment': [element.to_json() for element in self.alignment],
        }

    @staticmethod
    def from_json(fields: dict) -> 'WERData':
        return WERData(
            errors_count=fields['errors'],
            ref_words_count=fields['ref_wc'],
            hyp_words_count=fields['hyp_wc'],
            diff_hyp=fields['diff_hyp'],
            diff_ref=fields['diff_ref'],
            alignment=[AlignmentElement.from_json(element) for element in fields['alignment']],
        )


class WER(Metric):
    engine: CachingWEREngine
    cr: ClusterReference

    def __init__(self, name: str, engine: CachingWEREngine, cr: ClusterReference):
        super(WER, self).__init__(name=name)
        self.engine = engine
        self.cr = cr

    def get_metric_data(self, hyp: str, ref: str) -> WERData:
        errors_count, _, raw_alignment = self.engine(hyp, ref, self.cr)
        diff_hyp, diff_ref = visualize_raw_alignment(raw_alignment)
        alignment = [AlignmentElement.from_alice_named_tuple(element) for element in raw_alignment]
        ref_words_count, hyp_words_count = calculate_real_ref_hyp_word_counts(alignment)
        return WERData(errors_count, ref_words_count, hyp_words_count, diff_hyp, diff_ref, alignment)

    @staticmethod
    def calculate_metric(metric_data: WERData) -> float:
        # We want normalized value that is limited to 1. So this is not a vanilla WER.
        return metric_data.errors_count / max(metric_data.ref_words_count, metric_data.hyp_words_count, 1)

    @staticmethod
    def aggregate_metric(
        metric_value_list: typing.List[float],
        metric_data_list: typing.List[WERData],
    ) -> typing.Dict[str, float]:
        errors_count_total = 0
        ref_words_count_total = 0
        hyp_words_count_total = 0
        for metric_data in metric_data_list:
            errors_count_total += metric_data.errors_count
            ref_words_count_total += metric_data.ref_words_count
            hyp_words_count_total += metric_data.hyp_words_count
        total = WER.calculate_metric(WERData(errors_count_total, ref_words_count_total, hyp_words_count_total))
        mean = sum(metric_value_list) / len(metric_value_list)
        return {
            'mean': mean,
            'total': total,
        }

    @staticmethod
    def are_stop_words_applicable() -> bool:
        return True


# TODO: better way is to add hyp_len to Alice aligner, see https://a.yandex-team.ru/review/1477742/details
def calculate_real_ref_hyp_word_counts(alignment: typing.List[AlignmentElement]) -> typing.Tuple[int, int]:
    ref_len = 0
    hyp_len = 0
    for alignment_entry in alignment:
        if alignment_entry.action in [
            AlignmentAction.COINCIDENCE,
            AlignmentAction.SUBSTITUTION,
            AlignmentAction.DELETION,
        ]:
            ref_len += 1
        if alignment_entry.action in [
            AlignmentAction.COINCIDENCE,
            AlignmentAction.SUBSTITUTION,
            AlignmentAction.INSERTION,
        ]:
            hyp_len += 1
    return ref_len, hyp_len
