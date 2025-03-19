#!/usr/bin/python3
# coding: utf-8

import argparse
from pydub import AudioSegment
from pydub.exceptions import CouldntDecodeError
import os


def _parse_args():
    parser = argparse.ArgumentParser(
        description='Split long wav to small wavs',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        '--wav',
        help='path to wav file'
    )
    parser.add_argument(
        '--length',
        type=int,
        help='length of single part, ms'
    )
    parser.add_argument(
        '--offset',
        type=int,
        help='offset of splitting, ms',
    )
    parser.add_argument(
        '--outdir',
        help='path to directory for results'
    )

    return parser.parse_args()


def get_file_name(base, left, right):
    return base + '_' + str(left) + '-' + str(right) + '.wav'


def write_audio(audio, file_path):
    with open(file_path, 'wb') as f:
        audio.export(f, format='wav')


def write_small_wav(file_name, base, left, right, outdir):
    audio = base[left:right]
    name = get_file_name(file_name, left, right)
    print(name)
    write_audio(audio, os.path.join(outdir, name))


def split_wav(wav, length, offset, outdir):
    assert length >= offset, 'length should be >= than offset'

    file_name, extension = os.path.splitext(os.path.basename(wav))
    assert extension == '.wav', 'file isn\'t in .wav format'

    try:
        base = AudioSegment.from_wav(wav)
    except CouldntDecodeError as e:
        print('Failed to decode: %s' % wav)
        print(e)
        return

    wav_len = base.__len__()

    print("Source size, ms : " + str(wav_len))

    if (wav_len - length) % offset != 0:
        print("Silence added, ms : " + str(offset - (wav_len - length) % offset))
        base += AudioSegment.silent(offset - (wav_len - length) % offset, base.frame_rate)
        wav_len = base.__len__()
        print("Final size, ms : " + str(wav_len))

    for i in range(0, wav_len - length + 1, offset):
        write_small_wav(file_name, base, i, i + length, outdir)
