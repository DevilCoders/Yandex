import numpy as np
import torch
from torch import Tensor
import torch.nn as nn
from torch.nn.utils.rnn import pad_sequence

from acoustic_model.aligner import b_mas, ConvAttention
from acoustic_model.prior_attn import prior_attn
from acoustic_model.data import AcousticBatch
from acoustic_model.data.text_processor import TextProcessorBase
from core.models import (
    Embedding,
    PositionalEncoding,
    TransformerDecoder,
    TransformerEncoder
)
from core.utils import mask_from_lengths


class TemporalPredictor(nn.Module):
    def __init__(self, config: dict):
        super().__init__()
        self.transformer = TransformerEncoder(**config)
        self.final_linear = nn.Linear(self.transformer.dim(), 1)

    def forward(self, x: Tensor) -> Tensor:
        x = self.transformer(x)
        x = self.final_linear(x)
        return x.squeeze(2)


class AcousticModel(nn.Module):
    def __init__(self,
                 model_config: dict,
                 speakers: dict,
                 text_processor: TextProcessorBase):
        super().__init__()
        self.mel_dim = model_config["mel_dim"]

        if "templates" in model_config:
            self.is_adaptive = True
            gap_embedding_dim = model_config["templates"]["coalescence_window_size"] * self.mel_dim
            self.gap_embedding = Embedding(1, gap_embedding_dim, padding_idx=None)
            self.template_encoder = TransformerEncoder(**model_config["templates"]["encoder"])
            self.encoder = TransformerDecoder(**model_config["encoder"])
        else:
            self.is_adaptive = False
            self.gap_embedding = self.template_encoder = None
            self.encoder = TransformerEncoder(**model_config["encoder"])

        if model_config.get("aligner", False):
            self.aligner = ConvAttention(self.mel_dim,
                                         None,
                                         self.encoder.dim(),
                                         n_att_channels=self.mel_dim,
                                         use_query_proj=True,
                                         align_query_enc_type="3xconv")
        else:
            self.aligner = None

        self.embedding = Embedding(text_processor.vocab_size, **model_config["embedding"])
        self.positional_encoding = PositionalEncoding(**model_config["positional_encoding"])
        self.accent_embedding = Embedding(2, self.encoder.dim())

        num_speakers = max(value for value in speakers["speaker_id"].values())
        if not speakers["role_id"].values():
            num_roles = 0
        else:
            num_roles = max(value for value in speakers["role_id"].values())
        self.speaker_embedding = Embedding(num_speakers + 1, self.encoder.dim())
        self.role_embedding = Embedding(num_roles + 1, self.encoder.dim())

        if "duration_estimator" in model_config:
            self.duration_estimator = TemporalPredictor(model_config["duration_estimator"])
        else:
            self.duration_estimator = None
        if "pitch_estimator" in model_config:
            self.pitch_estimator = TemporalPredictor(model_config["pitch_estimator"])
        else:
            self.pitch_estimator = self.pitch_embedding = None

        kernel_size = model_config["pitch_embedding"]["kernel_size"]
        self.pitch_embedding = nn.Conv1d(in_channels=1,
                                         out_channels=self.encoder.dim(),
                                         kernel_size=kernel_size,
                                         padding=int((kernel_size - 1) / 2))

        self.decoder = TransformerEncoder(**model_config["decoder"])

        self.mel_linear = None
        if model_config.get("mel_linear", False):
            self.mel_linear = nn.Linear(self.decoder.dim(), self.mel_dim)

        self.text_processor = text_processor

    def forward(self, batch: AcousticBatch):
        templates = None
        if self.is_adaptive:
            templates = self._insert_gap_embedding(batch.mel_templates)
            templates = self.template_encoder(templates)

        x = self.embedding(batch.tokens)
        x = self.positional_encoding(x)
        x = x + self.accent_embedding(batch.accents)
        x = x + self.speaker_embedding(batch.speaker_ids)
        x = x + self.role_embedding(batch.role_ids)

        mask = mask_from_lengths(batch.input_lengths).unsqueeze(2).to(x)
        x = x * mask

        if templates is not None:
            x = self.encoder(x, templates)
        else:
            x = self.encoder(x)

        if self.aligner is not None:
            # Alignment
            attn_mask = mask_from_lengths(batch.input_lengths)[..., None] == 0
            # attn_mask should be 1 for unused timesteps
            attn_soft, attn_logprob = self.aligner(batch.mels.transpose(1, 2),
                                                   x.transpose(1, 2),
                                                   batch.mel_lengths,
                                                   attn_mask,
                                                   attn_prior=prior_attn(batch.mel_lengths, batch.input_lengths))
            attn_hard = self._binarize_attention_parallel(attn_soft, batch.input_lengths, batch.mel_lengths)
            # Viterbi --> durations
            attn_hard_dur = attn_hard.sum(2)[:, 0, :]
            dur_tgt = attn_hard_dur
            assert torch.all(torch.eq(dur_tgt.sum(dim=1), batch.mel_lengths))
        else:
            attn_logprob = attn_soft = attn_hard = None
            dur_tgt = batch.durations

        if self.duration_estimator is not None:
            log_dur_pred = self.duration_estimator(x)
        else:
            log_dur_pred = None

        if self.pitch_estimator is not None:
            pitch_pred = self.pitch_estimator(x)
        else:
            pitch_pred = None

        x = x * mask

        # Average pitch over words
        pitch_tgt = _average_pitch(batch.tokens.cpu().numpy(),
                                   batch.pitch.cpu().numpy(),
                                   dur_tgt.cpu().long().numpy(),
                                   batch.input_lengths.cpu().numpy().reshape(-1),
                                   batch.mel_lengths.cpu().numpy().reshape(-1),
                                   self.text_processor)
        pitch_tgt = pad_sequence([torch.tensor(item) for item in pitch_tgt], batch_first=True).to(x)
        pitch_tgt = torch.log(pitch_tgt + 1)

        x = _repeat_tensor(x, dur_tgt)
        aligned_pitch = _repeat_tensor(pitch_tgt, dur_tgt)
        x = x + self.pitch_embedding(aligned_pitch.unsqueeze(1)).transpose(1, 2)

        x = self.decoder(x)
        if self.mel_linear is not None:
            x = self.mel_linear(x)

        model_output = x, log_dur_pred, pitch_pred, attn_logprob, attn_soft, attn_hard, dur_tgt, pitch_tgt

        return model_output

    def infer(self, batch: AcousticBatch):
        templates = None
        if self.is_adaptive:
            templates = self._insert_gap_embedding(batch.mel_templates)
            templates = self.template_encoder(templates)

        x = self.embedding(batch.tokens)
        x = self.positional_encoding(x)
        x = x + self.accent_embedding(batch.accents)
        x = x + self.speaker_embedding(batch.speaker_ids)
        x = x + self.role_embedding(batch.role_ids)

        mask = mask_from_lengths(batch.input_lengths).unsqueeze(2).to(x)
        x = x * mask

        if templates is not None:
            x = self.encoder(x, templates)
        else:
            x = self.encoder(x)

        log_dur_pred = self.duration_estimator(x)
        dur_pred = torch.clamp(torch.exp(log_dur_pred) - 1, 0, 100)
        pitch_pred = self.pitch_estimator(x)

        x = x * mask

        x = _repeat_tensor(x, dur_pred)
        aligned_pitch = _repeat_tensor(pitch_pred, dur_pred)
        x = x + self.pitch_embedding(aligned_pitch.unsqueeze(1)).transpose(1, 2)

        x = self.decoder(x)
        if self.mel_linear is not None:
            x = self.mel_linear(x)

        return x

    def _binarize_attention_parallel(self, attn, in_lens, out_lens):
        """For training purposes only. Binarizes attention with MAS.
           These will no longer recieve a gradient.
        Args:
            attn: B x 1 x max_mel_len x max_text_len
        """
        with torch.no_grad():
            attn_cpu = attn.data.cpu().numpy()
            attn_out = b_mas(attn_cpu, in_lens.cpu().numpy(),
                             out_lens.cpu().numpy(), width=1)
        return torch.from_numpy(attn_out).to(attn.get_device())

    def _insert_gap_embedding(self, x: torch.Tensor) -> torch.Tensor:
        x = x.clone()
        mask = (x == float("inf")).all(dim=2)
        num_gaps = mask.sum().item()
        gap = self.gap_embedding(torch.tensor([0] * num_gaps).to(x.device))
        x[mask] = gap
        return x


class InferenceEncoder(nn.Module):
    def __init__(self, model: AcousticModel):
        super().__init__()
        self.max_duration = 100
        self.encoder = model.encoder
        self.template_encoder = model.template_encoder
        self.duration_estimator = model.duration_estimator
        self.pitch_estimator = model.pitch_estimator

    def forward(self, x, template=None):
        if self.template_encoder is not None:
            template = self.template_encoder(template)
            x = self.encoder(x, template)
        else:
            x = self.encoder(x)
        estimated_durations = self.duration_estimator(x)
        estimated_durations = torch.clamp(torch.exp(estimated_durations) - 1, 0, self.max_duration)
        estimated_pitch = self.pitch_estimator(x)
        return x, estimated_durations, estimated_pitch


class InferenceDecoder(nn.Module):
    def __init__(self, model: AcousticModel):
        super().__init__()
        self.pitch_embedding = model.pitch_embedding
        self.decoder = model.decoder
        self.mel_linear = model.mel_linear
        self.mel_dim = model.mel_dim

    def forward(self, x, pitch):
        x += self.pitch_embedding(pitch.unsqueeze(1)).transpose(1, 2)
        x = self.decoder(x)
        if self.mel_linear is not None:
            x = self.mel_linear(x)
        return x


def _repeat_tensor(x: Tensor, durations: Tensor) -> Tensor:
    durations = torch.round(durations).long()
    x = pad_sequence([torch.repeat_interleave(x, duration, dim=0) for x, duration in zip(x, durations)],
                     batch_first=True)
    return x


def _average_pitch(
    sequence_batch: np.ndarray,
    pitch_batch: np.ndarray,
    durations_batch: np.ndarray,
    sequence_lengths: np.ndarray,
    pitch_lengths: np.ndarray,
    text_processor: TextProcessorBase
):
    batch_size = sequence_batch.shape[0]
    result = []
    for i in range(batch_size):
        sequence = sequence_batch[i, :sequence_lengths[i]]
        durations = durations_batch[i, :sequence_lengths[i]]
        pitch = pitch_batch[i, :pitch_lengths[i]]

        start = None
        averaged_pitch = []
        durations_cumsum = np.cumsum(np.pad(durations, (1, 0)))
        for j, token in enumerate(sequence):
            if text_processor.id_to_symbol[token] not in text_processor.phones:
                if start is not None:
                    a, b = durations_cumsum[start], durations_cumsum[j]
                    values = pitch[a:b][np.where(pitch[a:b] != 0.0)[0]]
                    mean_pitch = np.mean(values) if len(values) > 0 else 0.0
                    averaged_pitch.extend([mean_pitch] * (j - start) + [0])
                    start = None
                else:
                    averaged_pitch.append(0)
            else:
                start = j if start is None else start
        assert start is None, "pitch preprocessing error: 'start' must be None"
        assert len(averaged_pitch) == len(sequence), "pitch preprocessing error: pitch length mismatch"
        result.append(averaged_pitch)
    return result
