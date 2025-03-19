import itertools
import typing
from datetime import datetime

from dataclasses import dataclass
from enum import Enum

import dataforge.feedback_loop as feedback_loop

from cloud.ai.lib.python.datetime import format_datetime, parse_datetime
from cloud.ai.lib.python.serialization import YsonSerializable
from .common_markup import MarkupStep
from .recognitions import RecognitionEndpoint
from .records import MarkupPriorities
from .records_joins import Recognition, get_recognition


@dataclass
class MetricEvalRecordData:
    record_id: str
    channel: int
    slices: typing.List[str]
    metric_name: str
    text_transformations: str
    metric_value: float
    metric_data: dict  # TODO: precise types
    hypothesis: str
    reference: str

    def to_yson(self) -> dict:
        return {
            'record_id': self.record_id,
            'channel': self.channel,
            'slices': self.slices,
            'metric_name': self.metric_name,
            'text_transformations': self.text_transformations,
            'metric_value': self.metric_value,
            'metric_data': self.metric_data,
            'hypothesis': self.hypothesis,
            'reference': self.reference,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MetricEvalRecordData':
        return MetricEvalRecordData(
            record_id=fields['record_id'],
            channel=fields['channel'],
            slices=fields['slices'],
            metric_name=fields['metric_name'],
            text_transformations=fields['text_transformations'],
            metric_value=fields['metric_value'],
            metric_data=fields['metric_data'],
            hypothesis=fields['hypothesis'],
            reference=fields['reference'],
        )


@dataclass
class MetricEvalRecord:
    record_id: str
    channel: int
    slices: typing.List[str]
    metric_name: str
    text_transformations: str
    api: str  # TODO: use enum
    model: str
    method: str  # TODO: use enum
    eval_id: str
    metric_value: float
    metric_data: dict  # TODO: precise types
    calc_data: typing.Optional[dict]  # extra data applied during metric calculation, i.e. cluster references
    hypothesis: Recognition
    reference: Recognition
    recognition_endpoint: RecognitionEndpoint
    received_at: datetime
    other: typing.Any

    def to_yson(self) -> dict:
        return {
            'record_id': self.record_id,
            'channel': self.channel,
            'slices': self.slices,
            'metric_name': self.metric_name,
            'text_transformations': self.text_transformations,
            'api': self.api,
            'model': self.model,
            'method': self.method,
            'eval_id': self.eval_id,
            # float values without floating point implicitly converted to int during json deserialization
            'metric_value': float(self.metric_value),
            'metric_data': self.metric_data,
            'calc_data': self.calc_data,
            'hypothesis': self.hypothesis.to_yson(),
            'reference': self.reference.to_yson(),
            'recognition_endpoint': self.recognition_endpoint.to_yson(),
            'received_at': format_datetime(self.received_at),
            'other': self.other,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MetricEvalRecord':
        return MetricEvalRecord(
            record_id=fields['record_id'],
            channel=fields['channel'],
            slices=fields['slices'],
            metric_name=fields['metric_name'],
            text_transformations=fields['text_transformations'],
            api=fields['api'],
            model=fields['model'],
            method=fields['method'],
            eval_id=fields['eval_id'],
            metric_value=fields['metric_value'],
            metric_data=fields['metric_data'],
            calc_data=fields['calc_data'],
            hypothesis=get_recognition(fields['hypothesis']),
            reference=get_recognition(fields['reference']),
            recognition_endpoint=RecognitionEndpoint.from_yson(fields['recognition_endpoint']),
            received_at=parse_datetime(fields['received_at']),
            other=fields['other'],
        )

    def __lt__(self, other: 'MetricEvalRecord'):
        if self.record_id == other.record_id:
            if self.channel == other.channel:
                if self.metric_name == other.metric_name:
                    if self.text_transformations == other.text_transformations:
                        if self.api == other.api:
                            if self.model == other.model:
                                if self.method == other.method:
                                    return self.eval_id < other.eval_id
                                return self.method < other.method
                            else:
                                return self.model < other.model
                        else:
                            return self.api < other.api
                    else:
                        return self.text_transformations < other.text_transformations
                else:
                    return self.metric_name < other.metric_name
            else:
                return self.channel < other.channel
        else:
            return self.record_id < other.record_id


@dataclass
class MetricEvalTagData:
    tag: str
    slice: str
    metric_name: str
    aggregation: str
    text_transformations: str
    metric_value: float

    def to_yson(self) -> dict:
        return {
            'tag': self.tag,
            'slice': self.slice,
            'metric_name': self.metric_name,
            'aggregation': self.aggregation,
            'text_transformations': self.text_transformations,
            'metric_value': self.metric_value,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MetricEvalTagData':
        return MetricEvalTagData(
            tag=fields['tag'],
            slice=fields['slice'],
            metric_name=fields['metric_name'],
            aggregation=fields['aggregation'],
            text_transformations=fields['text_transformations'],
            metric_value=fields['metric_value'],
        )


@dataclass
class MetricEvalTag:
    tag: str
    slice: str
    metric_name: str
    aggregation: str
    text_transformations: str
    api: str  # TODO: use enum
    model: str
    method: str  # TODO: use enum
    eval_id: str
    metric_value: float
    calc_data: dict  # extra data applied during metric calculation, i.e. cluster references
    recognition_endpoint: RecognitionEndpoint
    received_at: datetime
    other: typing.Any

    def to_yson(self) -> dict:
        return {
            'tag': self.tag,
            'slice': self.slice,
            'metric_name': self.metric_name,
            'classification': self.aggregation,
            'text_transformations': self.text_transformations,
            'api': self.api,
            'model': self.model,
            'method': self.method,
            'eval_id': self.eval_id,
            # float values without floating point implicitly converted to int during json deserialization
            'metric_value': float(self.metric_value),
            'calc_data': self.calc_data,
            'recognition_endpoint': self.recognition_endpoint.to_yson(),
            'received_at': format_datetime(self.received_at),
            'received_at_ts': int(self.received_at.timestamp()),
            'other': self.other,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MetricEvalTag':
        return MetricEvalTag(
            tag=fields['tag'],
            slice=fields['slice'],
            metric_name=fields['metric_name'],
            aggregation=fields['classification'],
            text_transformations=fields['text_transformations'],
            api=fields['api'],
            model=fields['model'],
            method=fields['method'],
            eval_id=fields['eval_id'],
            metric_value=fields['metric_value'],
            calc_data=fields['calc_data'],
            recognition_endpoint=RecognitionEndpoint.from_yson(fields['recognition_endpoint']),
            received_at=parse_datetime(fields['received_at']),
            other=fields['other'],
        )

    def __lt__(self, other: 'MetricEvalTag'):
        if self.tag == other.tag:
            if self.slice == other.slice:
                if self.metric_name == other.metric_name:
                    if self.aggregation == other.aggregation:
                        if self.text_transformations == other.text_transformations:
                            if self.api == other.api:
                                if self.model == other.model:
                                    if self.method == other.method:
                                        return self.eval_id < other.eval_id
                                    return self.method < other.method
                                else:
                                    return self.model < other.model
                            else:
                                return self.api < other.api
                        else:
                            return self.text_transformations < other.text_transformations
                    else:
                        return self.aggregation < other.aggregation
                else:
                    return self.metric_name < other.metric_name
            else:
                return self.slice < other.slice
        else:
            return self.tag < other.tag


@dataclass
class MetricMarkupStep:
    pool_id: str
    cost: float
    toloker_speed_mean: float
    all_assignments_honeypots_quality_mean: typing.Optional[float]
    accepted_assignments_honeypots_quality_mean: typing.Optional[float]

    def to_yson(self) -> dict:
        return {
            'pool_id': self.pool_id,
            'cost': self.cost,
            'toloker_speed_mean': self.toloker_speed_mean,
            'all_assignments_honeypots_quality_mean': self.all_assignments_honeypots_quality_mean,
            'accepted_assignments_honeypots_quality_mean': self.accepted_assignments_honeypots_quality_mean,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MetricMarkupStep':
        return MetricMarkupStep(
            pool_id=fields['pool_id'],
            cost=fields['cost'],
            toloker_speed_mean=fields['toloker_speed_mean'],
            all_assignments_honeypots_quality_mean=fields['all_assignments_honeypots_quality_mean'],
            accepted_assignments_honeypots_quality_mean=fields['accepted_assignments_honeypots_quality_mean'],
        )


class MarkupType(Enum):
    REGULAR = 'regular'  # regular (i.e. daily) markups based on some fixed tag sets
    CUSTOM = 'custom'  # custom one-shot markup for some specific case
    EXTERNAL = 'external'  # markup for partner's data from DataSphere SpeechkitPro


@dataclass
class TagsMarkupStats:
    tags: typing.List[str]
    duration_seconds: float

    def to_yson(self) -> dict:
        return {
            'tags': self.tags,
            'duration_seconds': self.duration_seconds,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'TagsMarkupStats':
        return TagsMarkupStats(
            tags=fields['tags'],
            duration_seconds=fields['duration_seconds'],
        )


@dataclass
class MarkupMetadata:
    markup_id: str
    markup_priority: MarkupPriorities
    tags_stats: typing.List[TagsMarkupStats]
    started_at: datetime

    def to_yson(self) -> dict:
        return {
            'markup_id': self.markup_id,
            'markup_priority': self.markup_priority.value,
            'tags_stats': [s.to_yson() for s in self.tags_stats],
            'started_at': format_datetime(self.started_at),
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MarkupMetadata':
        return MarkupMetadata(
            markup_id=fields['markup_id'],
            markup_priority=MarkupPriorities(fields['markup_priority']),
            tags_stats=[TagsMarkupStats.from_yson(s) for s in fields['tags_stats']],
            started_at=parse_datetime(fields['started_at']),
        )

    def get_tags(self) -> typing.Iterable[str]:
        return set(itertools.chain.from_iterable(tags_stats.tags for tags_stats in self.tags_stats))


@dataclass
class RecordsBitsStats(YsonSerializable):
    duration_seconds_percentiles: dict  # typing.Dict[int, float]
    overlap_percentiles: dict  # typing.Dict[int, float]


percentiles = [10, 25, 50, 75, 90, 95, 99]


@dataclass
class MetricMarkup:
    markup_id: str
    markup_type: MarkupType
    markup_metadata: MarkupMetadata
    language: str
    records_durations_sum_minutes: float
    markup_duration_minutes: float
    filtered_transcript_bits_ratio: float
    accuracy: typing.Optional[float]
    not_evaluated_records_ratio: typing.Optional[float]
    record_markup_speed_mean: float
    step_to_metrics: typing.Dict[MarkupStep, typing.Optional[MetricMarkupStep]]
    transcript_feedback_loop: typing.Optional[feedback_loop.Metrics]
    bits_stats: RecordsBitsStats
    nirvana_workflow_url: str
    received_at: datetime

    def to_yson(self) -> dict:
        fields = {
            'markup_id': self.markup_id,
            'markup_type': self.markup_type.value,
            'markup_metadata': self.markup_metadata.to_yson(),
            'language': self.language,
            'records_durations_sum_minutes': self.records_durations_sum_minutes,
            'markup_duration_minutes': self.markup_duration_minutes,
            'filtered_transcript_bits_ratio': self.filtered_transcript_bits_ratio,
            'accuracy': self.accuracy,
            'not_evaluated_records_ratio': self.not_evaluated_records_ratio,
            'record_markup_speed_mean': self.record_markup_speed_mean,
            'step_to_metrics': {
                s.value: (None if m is None else m.to_yson())
                for s, m in self.step_to_metrics.items()
            },
            'transcript_feedback_loop':
                None if self.transcript_feedback_loop is None
                else self.transcript_feedback_loop.to_yson(),
            'bits_stats': self.bits_stats,
            'nirvana_workflow_url': self.nirvana_workflow_url,
            'received_at': format_datetime(self.received_at),
            'received_at_ts': int(self.received_at.timestamp()),
        }
        fields.update(self.generate_plain_markup_fields())
        return fields

    def __lt__(self, other: 'MetricMarkup'):
        return self.markup_id < other.markup_id

    def generate_plain_markup_fields(self) -> dict:
        fields = {}
        for step in [
            MarkupStep.CHECK_ASR_TRANSCRIPT,
            MarkupStep.TRANSCRIPT,
            MarkupStep.CHECK_TRANSCRIPT,
            MarkupStep.QUALITY_EVALUATION,
        ]:
            step_data = self.step_to_metrics.get(step)
            step_fields = {
                'cost': step_data.cost if step_data else None,
                'toloker_speed_mean': step_data.toloker_speed_mean if step_data else None,
                'all_assignments_honeypots_quality_mean': step_data.all_assignments_honeypots_quality_mean
                if step_data
                else None,
                'accepted_assignments_honeypots_quality_mean': step_data.accepted_assignments_honeypots_quality_mean
                if step_data
                else None,
            }
            for key, value in step_fields.items():
                fields[f'{step.value.replace("-", "_")}_{key}'] = value
        if self.transcript_feedback_loop is None:
            fields['transcript_feedback_loop_markup_attempts_eq_1'] = None
            fields['transcript_feedback_loop_markup_attempts_eq_2'] = None
            fields['transcript_feedback_loop_markup_attempts_eq_3'] = None
            fields['transcript_feedback_loop_markup_attempts_gt_3'] = None
            fields['transcript_feedback_loop_confidence_mean'] = None
            fields['transcript_feedback_loop_quality_unknown'] = None
            fields['transcript_feedback_loop_quality_ok'] = None
            fields['transcript_feedback_loop_quality_not_ok'] = None
        else:
            fields['transcript_feedback_loop_markup_attempts_eq_1'] = 0
            fields['transcript_feedback_loop_markup_attempts_eq_2'] = 0
            fields['transcript_feedback_loop_markup_attempts_eq_3'] = 0
            fields['transcript_feedback_loop_markup_attempts_gt_3'] = 0
            for attempts, count in self.transcript_feedback_loop.markup_attempts.items():
                if attempts > 3:
                    fields['transcript_feedback_loop_markup_attempts_gt_3'] += count
                else:
                    fields[f'transcript_feedback_loop_markup_attempts_eq_{attempts}'] = count
            fields['transcript_feedback_loop_confidence_mean'] = self.transcript_feedback_loop.confidence_percentiles[50]  # it's median actually
            fields['transcript_feedback_loop_quality_unknown'] = self.transcript_feedback_loop.quality.unknown
            fields['transcript_feedback_loop_quality_ok'] = self.transcript_feedback_loop.quality.ok
            fields['transcript_feedback_loop_quality_not_ok'] = self.transcript_feedback_loop.quality.not_ok
        for p in percentiles:
            fields[f'bit_duration_seconds_p{p}'] = self.bits_stats.duration_seconds_percentiles[p]
            fields[f'bit_overlap_p{p}'] = self.bits_stats.overlap_percentiles[p]
        return fields
