import typing as tp

from librosa.filters import mel as librosa_mel_fn
import torch
from torch import Tensor


class FeaturesExtractor:
    def __init__(self,
                 n_fft: int,
                 window_length: int,
                 hop_length: int,
                 sample_rate: int,
                 num_mel_bins: int,
                 min_frequency: int,
                 max_frequency: tp.Optional[int],
                 device: str):
        self._n_fft = n_fft
        self._window_length = window_length
        self._hop_length = hop_length
        self._sample_rate = sample_rate
        self._num_mel_bins = num_mel_bins
        self._min_frequency = min_frequency
        self._max_frequency = max_frequency
        self._device = device

        self._mel_basis = torch.from_numpy(librosa_mel_fn(self._sample_rate,
                                                          self._n_fft,
                                                          self._num_mel_bins,
                                                          self._min_frequency,
                                                          self._max_frequency)).float().to(self._device)
        self._window = torch.hann_window(window_length).float().to(device)

    def __call__(self, signal: Tensor) -> Tensor:
        pad_value = [int((self._n_fft - self._hop_length) // 2), int((self._n_fft - self._hop_length) // 2)]
        signal = torch.nn.functional.pad(signal.unsqueeze(1), pad_value, mode="reflect").squeeze(1)
        spectrogram = torch.stft(signal,
                                 n_fft=self._n_fft,
                                 hop_length=self._hop_length,
                                 win_length=self._window_length,
                                 window=self._window,
                                 center=False,
                                 pad_mode="reflect",
                                 normalized=False,
                                 onesided=True)
        spectrogram = torch.sqrt(spectrogram.pow(2).sum(-1) + 1e-9)
        spectrogram = torch.matmul(self._mel_basis, spectrogram)
        spectrogram = torch.log(torch.clamp(spectrogram, min=1e-5))
        return spectrogram
