from typing import List

import torch

from acoustic_model.data import AcousticBatch
from e2e_model.data.sample_builder import End2EndTrainSample


class End2EndBatch(AcousticBatch):
    def __init__(self, samples: List[End2EndTrainSample]):
        super().__init__([sample.acoustic_train_sample for sample in samples])
        max_audio_length = max(len(sample.audio) for sample in samples)
        self._audio = torch.zeros(self.batch_size, max_audio_length)
        for i, sample in enumerate(samples):
            self._audio[i, :len(sample.audio)] = sample.audio

    @property
    def audio(self):
        return self._audio


def collate_wrapper(batch):
    if isinstance(batch[0], list):
        assert len(batch) == 1
        return End2EndBatch(batch[0])
    return End2EndBatch(batch)
