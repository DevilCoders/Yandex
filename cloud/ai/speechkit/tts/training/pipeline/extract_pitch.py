import argparse
import glob
import json
import os

import librosa
import numpy as np
import soundfile as sf


def calculate_pitch(audio: np.ndarray, mel_length: int, min_freq: str, max_freq: str, frame_length: int) -> np.ndarray:
    pitch, voiced_flag, voiced_probs = librosa.pyin(audio,
                                                    fmin=librosa.note_to_hz(min_freq),
                                                    fmax=librosa.note_to_hz(max_freq),
                                                    frame_length=frame_length)
    assert np.abs(mel_length - pitch.shape[0]) <= 1.0
    pitch = np.where(np.isnan(pitch), 0.0, pitch)
    if len(pitch) < mel_length:
        pitch = np.pad(pitch, (0, mel_length - pitch))
    elif len(pitch) > mel_length:
        pitch = pitch[:mel_length]
    assert len(pitch) == mel_length
    return pitch


def process_row(wav_path_in: str, mel_path_in: str, pitch_path_out: str, json_path_out: str, config: dict):
    try:
        audio, sample_rate = sf.read(wav_path_in)
        assert len(audio.shape) == 1, 'mono audio expected'
        mel = np.load(mel_path_in)
        assert len(mel.shape) == 2, 'wrong mel shape length'
        if mel.shape[1] != config['num_mels']:
            mel = np.transpose(mel)
        assert mel.shape[1] == config['num_mels'], 'wrong mel dim'
        pitch = calculate_pitch(audio, len(mel), config['min_freq'], config['max_freq'], config['frame_length'])
        np.save(pitch_path_out, pitch)
    except Exception as e:
        json.dump({'error': f'{e}'}, open(json_path_out, 'w'))


def main(input_dir: str, output_dir: str, file_ext: str, config: dict):
    for wav_path_in in glob.glob(os.path.join(input_dir, '*.pcm.wav')):
        mel_path_in = wav_path_in.replace('.pcm.wav', '.mel.npy')
        pitch_path_out = os.path.join(output_dir, os.path.basename(wav_path_in).replace('.pcm.wav', file_ext))
        json_path_out = pitch_path_out.replace(file_ext, '.json')
        process_row(wav_path_in, mel_path_in, pitch_path_out, json_path_out, config)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input-dir', type=str, required=True)
    parser.add_argument('--output-dir', type=str, required=True)
    parser.add_argument('--min-freq', type=str, default='C2')
    parser.add_argument('--max-freq', type=str, default='C7')
    parser.add_argument('--frame-length', type=int, default=1024)
    parser.add_argument('--num-mels', type=int, default=80)
    parser.add_argument('--file-ext', type=str, default='.pitch.npy')
    args = parser.parse_args()

    config = {
        'min_freq': args.min_freq,
        'max_freq': args.max_freq,
        'frame_length': args.frame_length,
        'num_mels': args.num_mels
    }
    main(args.input_dir, args.output_dir, args.file_ext, config)
