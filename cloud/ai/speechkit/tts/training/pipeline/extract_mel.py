import argparse
import glob
import json
import os
import typing as tp

import librosa
import numpy as np
import soundfile as sf


def mel_spectrogram(y: np.ndarray,
                    sr: int,
                    n_fft: int,
                    win_length: int,
                    hop_length: int,
                    min_freq: int,
                    max_freq: tp.Optional[int],
                    num_mels: int) -> np.ndarray:
    mel_basis = librosa.filters.mel(sr,
                                    n_fft,
                                    n_mels=num_mels,
                                    fmin=min_freq,
                                    fmax=max_freq)
    x = librosa.stft(y,
                     n_fft=n_fft,
                     hop_length=hop_length,
                     win_length=win_length,
                     window='hann')
    x = np.abs(x)
    x = np.dot(mel_basis, x)
    x = np.log(np.maximum(1e-5, x))
    return x


def process_row(pcm_path_in: str,
                pcm_path_out: str,
                mel_path_out: str,
                mel_config: dict):
    audio, sr = sf.read(pcm_path_in, dtype='float32')
    if len(audio.shape) > 1:
        assert audio.shape[1] == 2
        audio = librosa.to_mono(audio.transpose())
        assert len(audio.shape) == 1
    audio = librosa.resample(audio, sr, mel_config['sr'])
    sf.write(pcm_path_out, audio, mel_config['sr'], subtype='PCM_16')
    mel = mel_spectrogram(y=audio,
                          sr=mel_config['sr'],
                          n_fft=mel_config['n_fft'],
                          win_length=mel_config['win_length'],
                          hop_length=mel_config['hop_length'],
                          min_freq=mel_config['min_freq'],
                          max_freq=mel_config['max_freq'],
                          num_mels=mel_config['num_mels'])
    np.save(mel_path_out, mel)


def extract_mel(input_dir: str, output_dir: str, mel_config: dict):
    for pcm_path_in in glob.glob(input_dir + '/*.pcm.wav'):
        pcm_path_out = os.path.join(output_dir, os.path.basename(pcm_path_in))
        mel_path_out = pcm_path_out.replace('.pcm.wav', '.mel.npy')
        json_path_out = pcm_path_out.replace('.pcm.wav', '.json')
        try:
            process_row(pcm_path_in, pcm_path_out, mel_path_out, mel_config)
        except Exception:
            json.dump({'error': 'failed to extract mel'}, open(json_path_out, 'w'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input-dir', type=str, required=True)
    parser.add_argument('--output-dir', type=str, required=True)
    parser.add_argument('--sr', type=int, default=22050)
    parser.add_argument('--n-fft', type=int, default=1024)
    parser.add_argument('--win-length', type=int, default=1024)
    parser.add_argument('--hop-length', type=int, default=256)
    parser.add_argument('--min-freq', type=int, default=0)
    parser.add_argument('--max-freq', type=int, default=None)
    parser.add_argument('--num-mels', type=int, default=80)
    args = parser.parse_args()

    mel_config = {
        'sr': args.sr,
        'n_fft': args.n_fft,
        'win_length': args.win_length,
        'hop_length': args.hop_length,
        'min_freq': args.min_freq,
        'max_freq': args.max_freq,
        'num_mels': args.num_mels
    }
    extract_mel(args.input_dir, args.output_dir, mel_config)
