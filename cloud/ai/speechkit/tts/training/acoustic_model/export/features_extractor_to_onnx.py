import argparse
import os

import librosa
from librosa.filters import mel as librosa_mel_fn
import torch


class FeaturesExtractor(torch.nn.Module):
    def __init__(self,
                 n_fft: int,
                 window_length: int,
                 hop_length: int,
                 sampling_rate: int,
                 num_mel_bins: int,
                 min_frequency: int,
                 max_frequency: int):
        super().__init__()
        self.n_fft = n_fft
        self.window_length = window_length
        self.hop_length = hop_length
        self.freq_cutoff = self.n_fft // 2 + 1
        self.register_buffer("window", getattr(torch, "hann_window")(self.window_length).float())

        mel_basis = librosa_mel_fn(
            sampling_rate, n_fft, num_mel_bins, min_frequency, max_frequency
        )
        mel_basis = torch.from_numpy(mel_basis).float()
        self.register_buffer("mel_basis", mel_basis)

        fourier_basis = torch.rfft(torch.eye(self.n_fft), signal_ndim=1, onesided=False)
        forward_basis = fourier_basis[:self.freq_cutoff].permute(2, 0, 1).reshape(-1, 1, fourier_basis.shape[1])
        forward_basis = forward_basis * torch.as_tensor(
            librosa.util.pad_center(self.window, self.n_fft), dtype=forward_basis.dtype
        )
        self.audio2mel = torch.nn.Conv1d(
            forward_basis.shape[1],
            forward_basis.shape[0],
            forward_basis.shape[2],
            bias=False,
            stride=self.hop_length
        ).requires_grad_(False)
        self.audio2mel.weight.copy_(forward_basis)

    def forward(self, signal: torch.Tensor) -> torch.Tensor:
        real, imag = self.audio2mel(signal.unsqueeze(1)).split(self.freq_cutoff, dim=1)
        magnitude = torch.sqrt(real ** 2 + imag ** 2)
        mel_output = torch.matmul(self.mel_basis, magnitude)
        log_mel_spectrogram = torch.log(torch.clamp(mel_output, min=1e-5))
        return log_mel_spectrogram.transpose(1, 2)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--n-fft", type=int, default=1024)
    parser.add_argument("--window-length", type=int, default=1024)
    parser.add_argument("--hop-length", type=int, default=256)
    parser.add_argument("--sampling-rate", type=int, default=22050)
    parser.add_argument("--n-mel-bins", type=int, default=80)
    parser.add_argument("--min-frequency", type=int, default=0)
    parser.add_argument("--max-frequency", type=int, default=8000)
    parser.add_argument("--opset-version", type=int, default=10)
    parser.add_argument("--output-dir", type=str, default="onnx_models")
    args = parser.parse_args()

    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    features_extractor = FeaturesExtractor(n_fft=args.n_fft,
                                           window_length=args.window_length,
                                           hop_length=args.hop_length,
                                           sampling_rate=args.sampling_rate,
                                           num_mel_bins=args.n_mel_bins,
                                           min_frequency=args.min_frequency,
                                           max_frequency=args.max_frequency)

    with torch.no_grad():
        batch_size = 1
        num_samples = args.sampling_rate * 16

        dummy_signal = torch.randn(batch_size, num_samples)
        output = features_extractor(dummy_signal)

    input_names = ["audio"]
    output_names = ["spectrogram"]
    dynamic_axes = {
        "audio": {
            0: "batch_size",
            1: "num_samples"
        },
        "spectrogram": {
            0: "batch_size",
            1: "num_frames"
        }
    }

    torch.onnx.export(features_extractor,
                      dummy_signal,
                      os.path.join(args.output_dir, "features_extractor.onnx"),
                      opset_version=args.opset_version,
                      verbose=True,
                      export_params=True,
                      input_names=input_names,
                      output_names=output_names,
                      dynamic_axes=dynamic_axes)


if __name__ == "__main__":
    main()
