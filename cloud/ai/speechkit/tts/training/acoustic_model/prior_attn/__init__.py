import torch
from torch import Tensor

from .prior_attn_cuda import prior_attn as prior_attn_cuda


def prior_attn(mel_lengths: Tensor, text_lengths: Tensor) -> Tensor:
    assert len(mel_lengths) == len(text_lengths)
    batch_size = len(mel_lengths)
    max_mel_length = mel_lengths.max()
    max_text_length = text_lengths.max()
    output = torch.zeros(batch_size, max_mel_length, max_text_length).cuda()
    prior_attn_cuda(output, mel_lengths, text_lengths)
    return output
