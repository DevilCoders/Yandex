import typing

from dataclasses import dataclass
from enum import Enum

from cloud.ai.lib.python.serialization import YsonSerializable


class InputVersion(Enum):
    V1 = 'v1'  # "ref" is string, only mono-channel records
    V2 = 'v2'  # "ref" is list of strings, added support for multi-channel records


@dataclass
class Record:
    id: str
    tags: typing.List[str]
    reference: typing.List[str]  # text per record channel

    @staticmethod
    def from_yson(fields: dict, input_version: InputVersion) -> 'Record':
        if input_version == InputVersion.V1:
            reference = [fields['ref']]
        elif input_version == InputVersion.V2:
            reference = fields['ref']
        else:
            raise ValueError(f'Unexpected input version: {input_version}')
        return Record(
            id=fields['id'],
            tags=fields['tags'],
            reference=reference,
        )


@dataclass
class EvaluationTarget(YsonSerializable):
    record_id: str
    channel: int
    hypothesis: str
    reference: str
    slices: typing.List[str]

    def clone(self) -> 'EvaluationTarget':
        return EvaluationTarget(
            record_id=self.record_id,
            channel=self.channel,
            hypothesis=self.hypothesis,
            reference=self.reference,
            slices=list(self.slices),
        )


class TextTransformationStep(Enum):
    Normalize = 'norm'
    Lemmatize = 'lemm'
    RemovePunctuation = 'rem-punct'


@dataclass
class MetricsConfig:
    metric_name_to_text_transformation_cases: typing.Dict[str, typing.List[typing.List[TextTransformationStep]]]

    @staticmethod
    def from_yson(fields: dict) -> 'MetricsConfig':
        return MetricsConfig(
            metric_name_to_text_transformation_cases={
                metric_name: [
                    [TextTransformationStep(step) for step in text_process_steps]
                    for text_process_steps in text_process_cases
                ]
                for metric_name, text_process_cases in fields.items()
            }
        )
