import json
import typing
from datetime import datetime
from dataclasses import dataclass

from cloud.ai.lib.python.serialization import YsonSerializable


@dataclass
class Audio(YsonSerializable):
    url: str  # usually is S3 URL


@dataclass
class Text(YsonSerializable):
    text: str


# Some ASR model, either Yandex or from some of our competitors. Version, model, mode are all contained somehow in data.
@dataclass
class ASR(YsonSerializable):
    data: dict
    received_at: datetime


@dataclass
class OldASRPipelineAudioSource(YsonSerializable):
    tags: typing.List[str]
    data: dict

    def __eq__(self, other):
        return self.tags == other.tags and self.data == other.data

    def __hash__(self):
        return hash((json.dumps(self.tags), json.dumps(self.data)))


AudioSource = typing.Union[OldASRPipelineAudioSource]


@dataclass
class OldASRPipelineTranscriptSource(YsonSerializable):
    data: dict

    def __eq__(self, other):
        return self.data == other.data

    def __hash__(self):
        return hash(json.dumps(self.data))


TranscriptSource = typing.Union[OldASRPipelineTranscriptSource]
