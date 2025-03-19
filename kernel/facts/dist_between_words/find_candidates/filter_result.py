#!/usr/bin/env python
# -*- encoding:utf-8 -*-

from __future__ import print_function
import argparse
import yt.wrapper as yt

FACTOR_COLS = [
    "host5",
    "host10",
    "urls5",
    "urls10"
]


def get_args():
    parser = argparse.ArgumentParser(description='Filter result data to reduce final trie size')
    parser.add_argument('-i', '--input', help='Input table in format word1;word2;factors...', required=True)
    parser.add_argument('--filtered_table', help='Path to directory with filtered results', required=True)
    parser.add_argument('--output', help='Output MR directory', required=True)
    return parser.parse_args()


def filter_data(rec):
    is_empty_factors = True
    for col in FACTOR_COLS:
        if rec[col] != 0:
            is_empty_factors = False
            break
    is_all_digits = rec['word1'].isdigit() or rec['word2'].isdigit()
    yield yt.create_table_switch(int(is_empty_factors or is_all_digits))
    yield rec



def main():
    cfg = get_args()
    yt.run_map(filter_data, cfg.input, [cfg.output, cfg.filtered_table])


if __name__ == "__main__":
    main()
