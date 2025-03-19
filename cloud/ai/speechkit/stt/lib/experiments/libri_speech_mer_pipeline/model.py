import typing
from dataclasses import dataclass
from datetime import datetime

from cloud.ai.lib.python.datetime import format_datetime, parse_datetime
from cloud.ai.speechkit.stt.lib.data.model.dao import SbSChoice, MarkupData


@dataclass
class NoiseData:
    level_idx: int  # to compare ints not floats
    level: float
    variant: int
    dataset: str
    records: typing.List[str]

    def to_yson(self) -> dict:
        return {
            'level_idx': self.level_idx,
            'level': self.level,
            'variant': self.variant,
            'dataset': self.dataset,
            'records': self.records,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'NoiseData':
        return NoiseData(
            level_idx=fields['level_idx'],
            level=fields['level'],
            variant=fields['variant'],
            dataset=fields['dataset'],
            records=fields['records'],
        )


@dataclass
class Recognition:
    id: str
    dataset: str
    model: str
    record: str
    noise: typing.Optional[NoiseData]
    reference: str
    hypothesis: str
    received_at: datetime

    def __lt__(self, other: 'Recognition'):
        return self.id < other.id

    def sort_key(self):
        return self.id

    def to_yson(self) -> dict:
        return {
            'id': self.id,
            'dataset': self.dataset,
            'model': self.model,
            'record': self.record,
            'noise': self.noise.to_yson() if self.noise else None,
            'reference': self.reference,
            'hypothesis': self.hypothesis,
            'received_at': format_datetime(self.received_at),
        }

    @staticmethod
    def from_yson(fields: dict) -> 'Recognition':
        return Recognition(
            id=fields['id'],
            dataset=fields['dataset'],
            model=fields['model'],
            record=fields['record'],
            noise=NoiseData.from_yson(fields['noise']) if fields['noise'] else None,
            reference=fields['reference'],
            hypothesis=fields['hypothesis'],
            received_at=parse_datetime(fields['received_at']),
        )


@dataclass
class MarkupMetrics:
    defined_prior_accuracy: float
    undefined_prior_skew: float
    honeypots_accuracy: float

    def to_yson(self) -> dict:
        return {
            'defined_prior_accuracy': self.defined_prior_accuracy,
            'undefined_prior_skew': self.undefined_prior_skew,
            'honeypots_accuracy': self.honeypots_accuracy,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MarkupMetrics':
        return MarkupMetrics(
            defined_prior_accuracy=fields['defined_prior_accuracy'],
            undefined_prior_skew=fields['undefined_prior_skew'],
            honeypots_accuracy=fields['honeypots_accuracy'],
        )


@dataclass
class MarkupSbS:
    recognition_id_left: str
    recognition_id_right: str
    dataset: str
    record: str
    reference: str
    hypothesis_left: str
    hypothesis_right: str
    prior: typing.Optional[SbSChoice]
    choice: SbSChoice
    model_left: str
    model_right: str
    noise_left: typing.Optional[NoiseData]
    noise_right: typing.Optional[NoiseData]
    pool_id: str
    assignment_id: str
    task: MarkupData
    markup_metrics: MarkupMetrics
    received_at: datetime

    def __lt__(self, other: 'MarkupSbS'):
        if self.recognition_id_left == other.recognition_id_left:
            return self.recognition_id_right < other.recognition_id_right
        else:
            return self.recognition_id_left < other.recognition_id_left

    def sort_key(self):
        return self.recognition_id_left + self.recognition_id_right

    def to_yson(self) -> dict:
        return {
            'recognition_id_left': self.recognition_id_left,
            'recognition_id_right': self.recognition_id_right,
            'dataset': self.dataset,
            'record': self.record,
            'reference': self.reference,
            'hypothesis_left': self.hypothesis_left,
            'hypothesis_right': self.hypothesis_right,
            'prior': self.prior.value if self.prior else None,
            'choice': self.choice.value,
            'model_left': self.model_left,
            'model_right': self.model_right,
            'noise_left': self.noise_left.to_yson() if self.noise_left else None,
            'noise_right': self.noise_right.to_yson() if self.noise_right else None,
            'pool_id': self.pool_id,
            'assignment_id': self.assignment_id,
            'task': self.task.to_yson(),
            'markup_metrics': self.markup_metrics.to_yson(),
            'received_at': format_datetime(self.received_at),
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MarkupSbS':
        return MarkupSbS(
            recognition_id_left=fields['recognition_id_left'],
            recognition_id_right=fields['recognition_id_right'],
            dataset=fields['dataset'],
            record=fields['record'],
            reference=fields['reference'],
            hypothesis_left=fields['hypothesis_left'],
            hypothesis_right=fields['hypothesis_right'],
            prior=SbSChoice(fields['prior']) if fields['prior'] else None,
            choice=SbSChoice(fields['choice']),
            model_left=fields['model_left'],
            model_right=fields['model_right'],
            noise_left=NoiseData.from_yson(fields['noise_left']) if fields['noise_left'] else None,
            noise_right=NoiseData.from_yson(fields['noise_right']) if fields['noise_right'] else None,
            pool_id=fields['pool_id'],
            assignment_id=fields['assignment_id'],
            task=MarkupData.from_yson(fields['task']),
            markup_metrics=MarkupMetrics.from_yson(fields['markup_metrics']),
            received_at=parse_datetime(fields['received_at']),
        )


@dataclass
class MarkupCheck:
    recognition_id: str
    dataset: str
    record: str
    reference: str
    hypothesis: str
    ok: bool
    model: str
    noise: typing.Optional[NoiseData]
    pool_id: str
    assignment_id: str
    task: MarkupData
    received_at: datetime

    def __lt__(self, other: 'MarkupCheck'):
        return self.recognition_id < other.recognition_id

    def sort_key(self):
        return self.recognition_id + self.recognition_id

    def to_yson(self) -> dict:
        return {
            'recognition_id': self.recognition_id,
            'dataset': self.dataset,
            'record': self.record,
            'reference': self.reference,
            'hypothesis': self.hypothesis,
            'ok': self.ok,
            'model': self.model,
            'noise': self.noise.to_yson() if self.noise else None,
            'pool_id': self.pool_id,
            'assignment_id': self.assignment_id,
            'task': self.task.to_yson(),
            'received_at': format_datetime(self.received_at),
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MarkupCheck':
        return MarkupCheck(
            recognition_id=fields['recognition_id'],
            dataset=fields['dataset'],
            record=fields['record'],
            reference=fields['reference'],
            hypothesis=fields['hypothesis'],
            ok=fields['ok'],
            model=fields['model'],
            noise=NoiseData.from_yson(fields['noise']) if fields['noise'] else None,
            pool_id=fields['pool_id'],
            assignment_id=fields['assignment_id'],
            task=MarkupData.from_yson(fields['task']),
            received_at=parse_datetime(fields['received_at']),
        )
