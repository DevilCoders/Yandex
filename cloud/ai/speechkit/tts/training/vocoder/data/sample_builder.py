from dataclasses import dataclass
import io
import math
import random
from typing import Tuple

import numpy as np
import soundfile as sf
import torch
from torch import Tensor

from core.data import RawTtsSample, TrainSample, TrainSampleBuilder


@dataclass(frozen=True)
class VocoderTrainSample(TrainSample):
    audio: Tensor
    mel: Tensor

    @property
    def group_key(self) -> float:
        return self.mel.size(1)


class VocoderSampleBuilder(TrainSampleBuilder):
    def __init__(
        self,
        sample_rate: int,
        num_mel_bins: int,
        segment_size: int,
        hop_length: int
    ):
        self._sample_rate = sample_rate
        self._num_mel_bins = num_mel_bins
        self._segment_size = segment_size
        self._hop_length = hop_length

    def __call__(self, raw_sample: RawTtsSample) -> VocoderTrainSample:
        audio, sample_rate = _load_audio(raw_sample.wav)
        assert sample_rate == self._sample_rate, f"bad sample rate: {sample_rate}"
        audio = torch.from_numpy(audio).float().unsqueeze(0)

        mel = raw_sample.mel
        if raw_sample.mel.shape[0] != self._num_mel_bins:
            mel = mel.transpose().copy()
        assert mel.shape[0] == self._num_mel_bins, f"bad mel shape: {mel.shape}"
        mel = torch.from_numpy(mel).unsqueeze(0)

        frames_per_segment = math.ceil(self._segment_size / self._hop_length)

        if audio.size(1) >= self._segment_size:
            mel_start = random.randint(0, mel.size(2) - frames_per_segment - 1)
            mel = mel[:, :, mel_start:mel_start + frames_per_segment]
            audio = audio[:, mel_start * self._hop_length:(mel_start + frames_per_segment) * self._hop_length]
        else:
            mel = torch.nn.functional.pad(mel, [0, frames_per_segment - mel.size(2)], "constant")
            audio = torch.nn.functional.pad(audio, [0, self._segment_size - audio.size(1)], "constant")

        return VocoderTrainSample(audio=audio, mel=mel.squeeze())


def _load_audio(wav: bytes) -> Tuple[np.ndarray, int]:
    audio, sample_rate = sf.read(io.BytesIO(wav), dtype="float32")
    return audio, sample_rate
