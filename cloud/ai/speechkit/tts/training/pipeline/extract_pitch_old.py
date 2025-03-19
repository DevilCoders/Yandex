import argparse
import glob
import json
import os

import numpy as np
import parselmouth
import soundfile as sf


def calculate_pitch(audio: np.ndarray, sample_rate: int, mel_length: int) -> np.ndarray:
    sound = parselmouth.Sound(audio, sample_rate)
    pitch = sound.to_pitch(time_step=sound.duration / (mel_length + 3)).selected_array['frequency']
    assert np.abs(mel_length - pitch.shape[0]) <= 1.0
    pitch = pitch[:min(len(pitch), mel_length)]
    return pitch


def process_row(wav_path_in: str, mel_path_in: str, pitch_path_out: str, json_path_out: str, num_mels: int):
    try:
        audio, sample_rate = sf.read(wav_path_in)
        assert len(audio.shape) == 1, 'mono audio expected'
        mel = np.load(mel_path_in)
        assert len(mel.shape) == 2, 'wrong mel shape length'
        if mel.shape[1] != num_mels:
            mel = np.transpose(mel)
        assert mel.shape[1] == num_mels, 'wrong mel dim'
        pitch = calculate_pitch(audio, sample_rate, len(mel))
        np.save(pitch_path_out, pitch)
    except Exception as e:
        json.dump({'error': f'{e}'}, open(json_path_out, 'w'))


def main(input_dir: str, output_dir: str, file_ext: str, num_mels: int):
    for wav_path_in in glob.glob(os.path.join(input_dir, '*.pcm.wav')):
        mel_path_in = wav_path_in.replace('.pcm.wav', '.mel.npy')
        pitch_path_out = os.path.join(output_dir, os.path.basename(wav_path_in).replace('.pcm.wav', file_ext))
        json_path_out = pitch_path_out.replace(file_ext, '.json')
        process_row(wav_path_in, mel_path_in, pitch_path_out, json_path_out, num_mels)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input-dir', type=str, required=True)
    parser.add_argument('--output-dir', type=str, required=True)
    parser.add_argument('--num-mels', type=int, default=80)
    parser.add_argument('--file-ext', type=str, default='.pitch.npy')
    args = parser.parse_args()

    main(args.input_dir, args.output_dir, args.file_ext, args.num_mels)
