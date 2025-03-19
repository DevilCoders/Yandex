#!/usr/bin/python3
# coding: utf-8

import argparse
import os
from cloud.ai.tools.audio.wav.lib.split.split import split_wav


def _parse_args():
    parser = argparse.ArgumentParser(
        description='Split long wavs in input directory to small wavs',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
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
        '--indir',
        help='path to directory with input'
    )
    parser.add_argument(
        '--outdir',
        help='path to directory for results'
    )

    return parser.parse_args()


def main():
    run(**vars(_parse_args()))


def run(length, offset, indir, outdir):
    for path in os.listdir(indir):
        wav = os.path.join(indir, path)
        if os.path.isfile(wav):
            split_wav(wav, length, offset, outdir)
            print('Done: ' + wav)
