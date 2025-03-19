import argparse
import glob
import json
import os
import subprocess as sp

import soundfile as sf


def ffmpeg_loudnorm(wav_path_in: str, wav_path_out: str, duration: float, sample_rate: int):
    cmd_base = 'ffmpeg -i ' + wav_path_in + ' -af '
    if duration < 3:
        cmd_base += 'apad,atrim=0:3,'
    cmd = cmd_base + f'loudnorm=I=-16:dual_mono=true:TP=-1.5:LRA=11:print_format=summary -ar {sample_rate} -f null -'
    output = sp.run(cmd.split(), capture_output=True)
    for line in output.stderr.decode().split('\n'):
        if 'Input Integrated' in line:
            input_i = float(line[len('Input Integrated:'):].strip().split()[0])
        elif 'Input True Peak' in line:
            input_tp = float(line[len('Input True Peak:'):].split()[0])
        elif 'Input LRA' in line:
            input_lra = float(line[len('Input LRA:'):].split()[0])
        elif 'Input Threshold' in line:
            input_thresh = float(line[len('Input Threshold:'):].split()[0])
        elif 'Target Offset' in line:
            target_offset = float(line[len('Target Offset:'):].split()[0])
    cmd = cmd_base + f'loudnorm=I=-16:TP=-1.5:LRA=11:measured_I={input_i}:measured_TP={input_tp}:measured_LRA={input_lra}:measured_thresh={input_thresh}:offset={target_offset}:linear=true:print_format=summary'
    if duration < 3:
        cmd += f',atrim=0:{duration} '
    cmd += f' -ar {sample_rate} {wav_path_out}'
    sp.run(cmd.split())


def process_row(json_path_out: str, wav_path_in: str, wav_path_out: str):
    try:
        audio, sample_rate = sf.read(wav_path_in, dtype='float32')
        duration = len(audio) / sample_rate
        ffmpeg_loudnorm(wav_path_in, wav_path_out, duration, sample_rate)
    except Exception as e:
        with open(json_path_out, 'w') as f:
            json.dump({'error': f'{e}'}, f)


def normalize_audio(input_dir: str, output_dir: str):
    for wav_path_in in glob.glob(os.path.join(input_dir, '*.pcm.wav')):
        wav_path_out = os.path.join(output_dir, os.path.basename(wav_path_in))
        json_path_out = wav_path_out.replace('.pcm.wav', '.json')
        process_row(json_path_out, wav_path_in, wav_path_out)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input-dir', type=str, required=True)
    parser.add_argument('--output-dir', type=str, required=True)
    args = parser.parse_args()

    normalize_audio(args.input_dir, args.output_dir)
