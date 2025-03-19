from dataclasses import dataclass
import re
import sys
from typing import List, Optional, Tuple

import numpy as np
import torch
from torch import Tensor

from acoustic_model.data.masking import create_mask, DataMask
from acoustic_model.data.text_processor import TextProcessorBase
from core.data import RawTtsSample, TrainSample, TrainSampleBuilder


@dataclass(frozen=True)
class AcousticTrainSample(TrainSample):
    mel: Tensor
    mel_template: Optional[Tensor]
    tokens: Tensor
    durations: Tensor
    accents: Tensor
    speaker_id: Tensor
    role_id: Tensor
    pitch: Tensor

    @property
    def group_key(self) -> float:
        return len(self.mel)


class AcousticSampleBuilder(TrainSampleBuilder):
    def __init__(self,
                 text_processor: TextProcessorBase,
                 variables_config: Optional[dict],
                 speakers: dict):
        self._text_processor = text_processor
        self._speakers = speakers
        self._is_adaptive = False
        if variables_config is not None:
            self._is_adaptive = True
            self._mel_dim = variables_config["mel_dim"]
            self._window_size = variables_config["coalescence_window_size"]
            self._stride = variables_config["coalescence_stride"]
            if "noise" in variables_config["mask_config"]:
                self._noise_mean = variables_config["mask_config"]["noise"]["mean"]
                self._noise_std = variables_config["mask_config"]["noise"]["std"]
            else:
                self._noise_mean = self._noise_std = None
            self._mask: DataMask = create_mask(text_processor, variables_config["mask_config"])

    def __call__(self, raw_sample: RawTtsSample) -> AcousticTrainSample:
        tokens, durations, accents = self._text_processor.decode_utterance(raw_sample.utterance, raw_sample.text, raw_sample.accented_text)
        tokens = self._text_processor.encode_symbols(tokens)

        speaker_id = self._speakers["speaker_id"][raw_sample.speaker]
        role_id = self._speakers["role_id"].get(raw_sample.speaker, 0)

        if self._is_adaptive:
            mask = self._mask(tokens, raw_sample.template_text, raw_sample.text_variables)
        else:
            mask = [0] * len(tokens)

        mel_length = raw_sample.mel.shape[-1]
        mask, durations = self._align(mask, durations, mel_length)
        assert len(durations) == len(tokens), "tokens length doesn't match durations length"

        mel = torch.from_numpy(raw_sample.mel).transpose(0, 1)
        mel_template = None
        if self._is_adaptive:
            mel_template = self._preprocess_template(mel, mask)

        assert len(tokens) <= len(mel), "tokens length must be less than or equal to mel length"

        return AcousticTrainSample(
            mel=mel,
            mel_template=mel_template,
            tokens=torch.LongTensor(tokens),
            durations=torch.LongTensor(durations),
            accents=torch.LongTensor(accents),
            speaker_id=torch.LongTensor([speaker_id]),
            role_id=torch.LongTensor([role_id]),
            pitch=torch.from_numpy(raw_sample.pitch),
        )

    def _preprocess_template(self, mel: Tensor, mask: List[int]) -> Tensor:
        coalesced_template = []
        begin = end = unmasked_part = None
        for bounds in self._get_unmasked_bounds(mask):
            begin, end = bounds
            unmasked_part = self._coalescence(mel[begin:end + 1, :])
            if len(coalesced_template) or (not len(coalesced_template) and begin > 0):
                coalesced_template.append(torch.empty(1, unmasked_part.size(1)).fill_(float("inf")))
            coalesced_template.append(unmasked_part)
        if len(coalesced_template) and end + 1 != len(mel):
            coalesced_template.append(torch.empty(1, unmasked_part.size(1)).fill_(float("inf")))

        if len(coalesced_template):
            coalesced_template = torch.cat(coalesced_template, 0)
        else:
            coalesced_template = torch.empty(1, self._window_size * self._mel_dim).fill_(float("inf"))

        return coalesced_template

    def _align(self,
               xs: List[int],
               durations: List[int],
               length: int) -> Tuple[List[int], List[int]]:
        durations = [duration / sum(durations) for duration in durations]

        residual = 0
        duration_frames = []
        aligned_xs = []
        for x, duration in zip(xs, durations):
            if not duration:
                duration_frames.append(0)
                continue
            duration += residual

            n_frames = length * duration
            n_frames_rounded = max(1, round(n_frames))
            duration_frames.append(n_frames_rounded)

            aligned_xs += [x] * n_frames_rounded
            residual = (n_frames - n_frames_rounded) / length

        assert sum(duration_frames) == length
        return aligned_xs, duration_frames

    def _coalescence(self, x: Tensor, pad_value: float = -11.5129):
        if self._window_size == 1 and self._stride == 1:
            return x

        x_length, x_dim = x.size()
        output = []
        for i in range(0, x_length, self._stride):
            j = min(i + self._window_size, x_length)
            if j - i < self._window_size:
                window = torch.zeros(self._window_size, x_dim).fill_(pad_value)
                window[:j - i, :] = x[i:j, :]
            else:
                window = x[i:j, :]
            output.append(window.reshape(1, -1))
        output = torch.cat(output, 0)

        return output

    def _get_noise(self) -> int:
        if self._noise_mean is None or self._noise_std is None:
            return 0
        return round(np.random.normal(self._noise_mean, self._noise_std))

    def _get_unmasked_bounds(self, mask: List[int]):
        bounds = []
        begin_pos = None
        i = 0
        for i, value in enumerate(mask):
            if value == 0 and begin_pos is None:
                begin_pos = i
            elif value == 1 and begin_pos is not None:
                if begin_pos > 0:
                    prev_end_pos = bounds[-1][1] if len(bounds) else 0
                    begin_pos = min(len(mask) - 1, max(prev_end_pos + 1, begin_pos + self._get_noise()))
                end_pos = max(begin_pos + 1, min(len(mask) - 1, i - 1 + self._get_noise()))
                bounds.append((begin_pos, end_pos))
                begin_pos = None
        if begin_pos is not None:
            bounds.append((begin_pos, i))
        return bounds
