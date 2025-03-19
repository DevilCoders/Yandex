from abc import ABC, abstractmethod
from dataclasses import dataclass

import numpy as np


@dataclass(frozen=True)
class RawTtsSample:
    id: str
    lang: str
    speaker: str
    text: str
    utterance: dict
    accented_text: str
    template_text: str
    text_variables: dict
    audio_variables: dict
    wav: bytes
    mel: np.ndarray
    pitch: np.ndarray


class TrainSample(ABC):
    @property
    @abstractmethod
    def group_key(self) -> float:
        pass


class TrainSampleBuilder(ABC):
    @abstractmethod
    def __call__(self, raw_sample: RawTtsSample) -> TrainSample:
        pass
