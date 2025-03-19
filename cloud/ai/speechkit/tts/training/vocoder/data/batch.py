from typing import List

import torch

from core.data import Batch
from vocoder.data.sample_builder import VocoderTrainSample


class VocoderBatch(Batch):
    def __init__(self, samples: List[VocoderTrainSample]):
        super().__init__(len(samples))
        self._audio = torch.stack(list(map(lambda sample: sample.audio, samples)))
        self._mel = torch.stack(list(map(lambda sample: sample.mel, samples)))

    @property
    def audio(self):
        return self._audio

    @property
    def mel(self):
        return self._mel


def collate_wrapper(batch):
    if isinstance(batch[0], list):
        assert len(batch) == 1
        return VocoderBatch(batch[0])
    return VocoderBatch(batch)
