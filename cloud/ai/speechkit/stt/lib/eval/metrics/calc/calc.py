import typing
from collections import defaultdict

from dataclasses import dataclass
from voicetech.asr.tools.asr_analyzer.lib.resources import ClusterReference, CachingWEREngine

from cloud.ai.speechkit.stt.lib.text.slice.application import PreparedSlice, infer_slices
from ..common import Metric, MetricData
from ..levenshtein import Levenshtein, CachingLevenshteinEngine
from ..mer import MER, CachingMEREngine
from ..wer import WER
from ...model import Record, EvaluationTarget


def get_metrics(cluster_references_path: str = '') -> typing.List[Metric]:
    return [get_metric_wer(cluster_references_path), get_metric_mer_1_0(), get_metric_lev()]


def get_metric_wer(cluster_references_path: str = '') -> WER:
    if cluster_references_path == '':
        print('Cluster references for WER is not specified')
        cr = None
    else:
        cr = ClusterReference(cr_file=cluster_references_path)
    return WER(name='WER', engine=CachingWEREngine(), cr=cr)


def get_metric_mer_1_0() -> MER:
    return MER(
        name='MER-1.0',
        engine=CachingMEREngine(
            features=[
                lambda w1, w2: -1.0 if w1.normal_form == w2.normal_form else 1.0,
                lambda w1, w2: -1.0 if w1.tag == w2.tag else 1.0,
                lambda w1, w2: -1.0 if w1.normal_form == "нет" else 1.0,
                lambda w1, w2: -1.0 if w2.normal_form == "нет" else 1.0,
                lambda w1, w2: -1.0 if w1.word == w2.word else 1.0,
                lambda w1, w2: -1.0 if w1.normal_form == "не" else 1.0,
                lambda w1, w2: -1.0 if w2.normal_form == "не" else 1.0,
                lambda w1, w2: -1.0 if w1.normal_form == "да" else 1.0,
                lambda w1, w2: -1.0 if w2.normal_form == "да" else 1.0,
                lambda w1, w2: 1.0,
            ],
            features_weights=[
                [-5.29136011e-03],
                [-3.16181456e-02],
                [-2.78462406e-02],
                [-6.57016586e-02],
                [1.79930783e00],
                [5.96907283e-02],
                [5.50059472e-02],
                [2.09496655e-01],
                [-1.26314612e-02],
                [1.00000000e-03],
            ],
        ),
    )


def get_metric_lev() -> Levenshtein:
    return Levenshtein(name='LEV', engine=CachingLevenshteinEngine())


@dataclass
class CalculateMetricResult:
    record_id_channel_to_metric_data: typing.Dict[typing.Tuple[str, int], MetricData]
    record_id_channel_to_metric_value: typing.Dict[typing.Tuple[str, int], float]
    tag_slice_to_metric_aggregates: typing.Dict[typing.Tuple[str, typing.Optional[str]], typing.Dict[str, float]]

    def __eq__(self, other: 'CalculateMetricResult'):
        def round_func(value):
            return float('{0:.5f}'.format(value))

        if self.record_id_channel_to_metric_data != other.record_id_channel_to_metric_data:
            return False

        if self.record_id_channel_to_metric_value.keys() != other.record_id_channel_to_metric_value.keys():
            return False

        for record_id_channel in self.record_id_channel_to_metric_value.keys():
            self_rounded_value = round_func(self.record_id_channel_to_metric_value[record_id_channel])
            other_rounded_value = round_func(other.record_id_channel_to_metric_value[record_id_channel])
            if self_rounded_value != other_rounded_value:
                return False

        if self.tag_slice_to_metric_aggregates.keys() != other.tag_slice_to_metric_aggregates.keys():
            return False

        for tag_slice in self.tag_slice_to_metric_aggregates.keys():
            self_metric_aggregates = self.tag_slice_to_metric_aggregates[tag_slice]
            other_metric_aggregates = other.tag_slice_to_metric_aggregates[tag_slice]
            if self_metric_aggregates.keys() != other_metric_aggregates.keys():
                return False
            for aggregate in self_metric_aggregates.keys():
                self_rounded_value = round_func(self_metric_aggregates[aggregate])
                other_rounded_value = round_func(other_metric_aggregates[aggregate])
                if self_rounded_value != other_rounded_value:
                    return False

        return True


def calculate_metric(
    metric: Metric,
    evaluation_targets: typing.List[EvaluationTarget],
    tag_to_records_ids: typing.Dict[str, typing.List[str]],
    stop_words: typing.Set[str],
) -> CalculateMetricResult:
    if not metric.are_stop_words_applicable():
        stop_words = None

    record_id_channel_to_metric_data = {}
    for evaluation_target in evaluation_targets:
        hyp, ref = (
            preprocess_text(t, stop_words)
            for t in (evaluation_target.hypothesis, evaluation_target.reference)
        )
        metric_data = metric.get_metric_data(hyp=hyp, ref=ref)
        record_id_channel_to_metric_data[(evaluation_target.record_id, evaluation_target.channel)] = metric_data

    record_id_channel_to_metric_value = {
        record_id_channel: metric.calculate_metric(metric_data)
        for record_id_channel, metric_data in record_id_channel_to_metric_data.items()
    }

    record_id_channel_to_slices = {
        (t.record_id, t.channel): t.slices for t in evaluation_targets
    }

    record_id_to_tags = defaultdict(list)
    for tag, records_ids in tag_to_records_ids.items():
        for record_id in records_ids:
            record_id_to_tags[record_id].append(tag)

    tag_slice_to_metric_data_list = defaultdict(list)
    tag_slice_to_metric_value_list = defaultdict(list)
    for record_id_channel in record_id_channel_to_metric_data.keys():
        metric_data = record_id_channel_to_metric_data[record_id_channel]
        metric_value = record_id_channel_to_metric_value[record_id_channel]

        record_id, _ = record_id_channel
        tags = record_id_to_tags[record_id]

        for tag in tags:
            slices = [None] + record_id_channel_to_slices[record_id_channel]
            for slice in slices:
                tag_slice_to_metric_data_list[(tag, slice)].append(metric_data)
                tag_slice_to_metric_value_list[(tag, slice)].append(metric_value)

    tag_slice_to_metric_aggregates = {}
    for tag_slice in tag_slice_to_metric_data_list.keys():
        metric_data_list = tag_slice_to_metric_data_list[tag_slice]
        metric_value_list = tag_slice_to_metric_value_list[tag_slice]
        metric_aggregates = metric.aggregate_metric(metric_value_list, metric_data_list)
        tag_slice_to_metric_aggregates[tag_slice] = metric_aggregates

    return CalculateMetricResult(
        record_id_channel_to_metric_data=record_id_channel_to_metric_data,
        record_id_channel_to_metric_value=record_id_channel_to_metric_value,
        tag_slice_to_metric_aggregates=tag_slice_to_metric_aggregates,
    )


def preprocess_text(
    text: str,
    stop_words: typing.Optional[typing.Set[str]] = None,
    replace_yo: bool = True,
) -> str:
    if stop_words is None or len(stop_words) == 0:
        stop_words = {'?'}

    # remove redundant whitespaces and stop words
    text = ' '.join(w for w in text.split(' ') if len(w) > 0 and w not in stop_words)

    text = text.lower()

    if replace_yo:
        # In markup we ask to type 'ё', so replace will be disabled in corresponding checks
        text = text.replace('ё', 'е')

    return text


def get_evaluation_targets(
    records: typing.List[Record],
    record_id_to_recognition: typing.Dict[str, typing.List[str]],
    slices: typing.List[PreparedSlice],
) -> typing.List[EvaluationTarget]:
    result = []
    for record in records:
        for i, reference in enumerate(record.reference):
            channel = i + 1
            result.append(
                EvaluationTarget(
                    record_id=record.id,
                    channel=channel,
                    hypothesis=record_id_to_recognition[record.id][i],
                    reference=reference,
                    slices=infer_slices(reference, slices),
                )
            )
    return result
