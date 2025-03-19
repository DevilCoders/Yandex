from typing import List

import torch

from acoustic_model.data.sample_builder import AcousticTrainSample
from core.data import Batch


class AcousticBatch(Batch):
    def __init__(self, samples: List[AcousticTrainSample]):
        super().__init__(len(samples))
        mel_dim = samples[0].mel.shape[1]
        max_mel_length = max(len(sample.mel) for sample in samples)
        self._mels = torch.zeros(self.batch_size, max_mel_length, mel_dim)
        self._mel_lengths = []

        self._mel_templates = self._mel_template_lengths = None
        if samples[0].mel_template is not None:
            mel_template_dim = samples[0].mel_template.shape[1]
            max_mel_template_length = max(len(sample.mel_template) for sample in samples)
            self._mel_templates = torch.zeros(self.batch_size, max_mel_template_length, mel_template_dim)
            self._mel_template_lengths = []

        max_input_length = max(len(sample.tokens) for sample in samples)

        self._tokens = torch.zeros(self.batch_size, max_input_length).long()
        self._durations = torch.zeros(self.batch_size, max_input_length)
        self._pitch = torch.zeros(self.batch_size, max_mel_length)
        self._accents = torch.zeros(self.batch_size, max_input_length).long()

        self._input_lengths = []
        self._speaker_ids = []
        self._role_ids = []

        for i, sample in enumerate(samples):
            mel_length = len(sample.mel)
            self._mels[i, :mel_length, :] = sample.mel
            self._mel_lengths.append(mel_length)

            if sample.mel_template is not None:
                self._mel_templates[i, :len(sample.mel_template), :] = sample.mel_template
                self._mel_template_lengths.append(len(sample.mel_template))

            input_length = len(sample.tokens)

            self._tokens[i, :input_length] = sample.tokens
            self._durations[i, :input_length] = sample.durations
            self._pitch[i, :mel_length] = sample.pitch
            self._accents[i, :input_length] = sample.accents
            self._input_lengths.append(input_length)

            self._speaker_ids.append(sample.speaker_id)
            self._role_ids.append(sample.role_id)

        self._mel_lengths = torch.tensor(self._mel_lengths).long().view(-1)
        if self._mel_template_lengths is not None:
            self._mel_template_lengths = torch.tensor(self._mel_template_lengths).long().view(-1)
        self._input_lengths = torch.tensor(self._input_lengths).long().view(-1)
        self._speaker_ids = torch.stack(self._speaker_ids)
        self._role_ids = torch.stack(self._role_ids)

    @property
    def mels(self):
        return self._mels

    @property
    def mel_templates(self):
        return self._mel_templates

    @property
    def tokens(self):
        return self._tokens

    @property
    def durations(self):
        return self._durations

    @property
    def pitch(self):
        return self._pitch

    @property
    def accents(self):
        return self._accents

    @property
    def speaker_ids(self):
        return self._speaker_ids

    @property
    def role_ids(self):
        return self._role_ids

    @property
    def mel_lengths(self):
        return self._mel_lengths

    @property
    def mel_template_lengths(self):
        return self._mel_template_lengths

    @property
    def input_lengths(self):
        return self._input_lengths


def collate_wrapper(batch):
    if isinstance(batch[0], list):
        assert len(batch) == 1
        return AcousticBatch(batch[0])
    return AcousticBatch(batch)
