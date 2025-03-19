import argparse
import glob
import json
import os

import librosa
from scipy.signal import butter, sosfiltfilt
import soundfile as sf


def process_row(json_path_out: str, wav_path_in: str, wav_path_out: str, config: dict):
    try:
        audio, sr = sf.read(wav_path_in, dtype='float32')
        if len(audio.shape) > 1:
            assert audio.shape[1] == 2
            audio = librosa.to_mono(audio.transpose())
            assert len(audio.shape) == 1
        nyq = sr * 0.5
        # Get the filter coefficients
        sos = butter(config['order'],
                     [config['lowpass_cutoff'] / nyq, config['hipass_cutoff'] / nyq],
                     btype=config['btype'],
                     analog=config['analog'],
                     output=config['output'])
        audio = sosfiltfilt(sos, audio)
        sf.write(wav_path_out, audio, sr, subtype='PCM_16')
    except Exception as e:
        with open(json_path_out, 'w') as f:
            json.dump({'error': f'{e}'}, f)


def audio_noise_reduction(input_dir: str, output_dir: str, config: dict):
    for wav_path_in in glob.glob(os.path.join(input_dir, '*.pcm.wav')):
        wav_path_out = os.path.join(output_dir, os.path.basename(wav_path_in))
        json_path_out = wav_path_out.replace('.pcm.wav', '.json')
        process_row(json_path_out, wav_path_in, wav_path_out, config)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input-dir', type=str, required=True)
    parser.add_argument('--output-dir', type=str, required=True)
    parser.add_argument('--order', type=int, default=5)
    parser.add_argument('--lowpass-cutoff', type=int, default=30)
    parser.add_argument('--hipass-cutoff', type=int, default=10500)
    parser.add_argument('--btype', type=str, default='bandpass')
    parser.add_argument('--analog', action='store_true')
    parser.add_argument('--output', type=str, default='sos')
    args = parser.parse_args()

    config = {
        'order': args.order,
        'lowpass_cutoff': args.lowpass_cutoff,
        'hipass_cutoff': args.hipass_cutoff,
        'btype': args.btype,
        'analog': args.analog,
        'output': args.output
    }
    audio_noise_reduction(args.input_dir, args.output_dir, config)
