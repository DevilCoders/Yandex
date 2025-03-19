from dataclasses import dataclass
from typing import Optional

import torch
from torch import Tensor

from acoustic_model.data import AcousticSampleBuilder, AcousticTrainSample
from acoustic_model.data.text_processor import TextProcessorBase
from core.data import RawTtsSample, TrainSample, TrainSampleBuilder
from vocoder.data.sample_builder import _load_audio


@dataclass(frozen=True)
class End2EndTrainSample(TrainSample):
    audio: Tensor
    acoustic_train_sample: AcousticTrainSample

    @property
    def group_key(self) -> float:
        return len(self.acoustic_train_sample.mel)


class End2EndSampleBuilder(TrainSampleBuilder):
    def __init__(
        self,
        text_processor: TextProcessorBase,
        variables_config: Optional[dict],
        speakers: dict,
        sample_rate: int
    ):
        self._acoustic_sample_builder = AcousticSampleBuilder(
            text_processor,
            variables_config,
            speakers
        )
        self._sample_rate = sample_rate

    def __call__(self, raw_sample: RawTtsSample) -> End2EndTrainSample:
        acoustic_train_sample = self._acoustic_sample_builder(raw_sample)
        audio, sample_rate = _load_audio(raw_sample.wav)
        assert sample_rate == self._sample_rate, f"bad sample rate: {sample_rate}"
        audio = torch.from_numpy(audio).float()
        return End2EndTrainSample(audio=audio, acoustic_train_sample=acoustic_train_sample)
