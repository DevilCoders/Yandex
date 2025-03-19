import argparse
import glob
import json
import os

import librosa
import noisereduce as nr
import soundfile as sf


def process_row(json_path_out: str, wav_path_in: str, wav_path_out: str, config: dict):
    try:
        audio, sr = sf.read(wav_path_in, dtype='float32')
        if len(audio.shape) > 1:
            assert audio.shape[1] == 2
            audio = librosa.to_mono(audio.transpose())
            assert len(audio.shape) == 1
        audio = nr.reduce_noise(y=audio,
                                sr=sr,
                                stationary=config['stationary'],
                                n_std_thresh_stationary=config['n_std_thresh_stationary'],
                                prop_decrease=config['prop_decrease'])
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
    parser.add_argument('--stationary', action='store_true')
    parser.add_argument('--n-std-thresh-stationary', type=float, default=0.1)
    parser.add_argument('--prop-decrease', type=float, default=1.0)
    args = parser.parse_args()

    config = {
        'stationary': args.stationary,
        'n_std_thresh_stationary': args.n_std_thresh_stationary,
        'prop_decrease': args.prop_decrease
    }
    audio_noise_reduction(args.input_dir, args.output_dir, config)
