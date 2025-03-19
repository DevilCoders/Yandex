# -*- coding: utf-8 -*-
import kernel.blender.factor_storage.pylib.serialization as serialization

import argparse
import json


def do_decompress(in_file, out_file):
    with open(in_file, 'r') as f:
        compressed_factors = f.read()
    error, static, dynamic = serialization.decompress(compressed_factors)
    assert error is None, 'error while decompressing: {}'.format(error)
    dump = {
        'static_factors': static,
        'dynamic_factors': dynamic
    }
    with open(out_file, 'w') as f:
        json.dump(dump, f)


def do_compress(in_file, out_file):
    with open(in_file, 'r') as f:
        factors = json.load(f)
    compressed = serialization.compress(factors['static_factors'], factors['dynamic_factors'])
    with open(out_file, 'wb') as f:
        f.write(compressed)


def parse_args():
    p = argparse.ArgumentParser(
        description='test program in python2 for parsing compressed factors',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    p.add_argument('--mode', required=True, choices=['decompress', 'compress'])
    p.add_argument('--in_file', required=True)
    p.add_argument('--out_file', required=True)

    return p.parse_args()


def main():
    args = parse_args()
    if args.mode == 'decompress':
        do_decompress(args.in_file, args.out_file)
    elif args.mode == 'compress':
        do_compress(args.in_file, args.out_file)
    else:
        raise ValueError('Unknown mode: {}'.format(args.mode))


if __name__ == '__main__':
    main()
