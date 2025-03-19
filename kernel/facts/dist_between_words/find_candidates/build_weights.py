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
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-i', '--input', help='Input table in format word1;word2;factors...'
                                              ';urls2', required=True)
    parser.add_argument('--output', help='Output MR directory', required=True)
    return parser.parse_args()


def reduce_factors(key, recs):
    total = [0.0] * len(FACTOR_COLS)
    amount = [0.0] * len(FACTOR_COLS)

    for rec in recs:
        for idx, col in enumerate(FACTOR_COLS):
            value = rec[col]
            total[idx] += 1
            amount[idx] += value

    res = {
        "word1": key["word1"],
        "word2": key["word2"],
        "freq": key["freq"]
    }
    is_empty_factors = True
    for idx, col in enumerate(FACTOR_COLS):
        res[col] = amount[idx] / total[idx]
        if res[col] != 0:
            is_empty_factors = False
    if not is_empty_factors:
        yield res


def main():
    cfg = get_args()
    yt.run_sort(cfg.input, cfg.input, sort_by=["word1", "word2"])
    yt.run_reduce(reduce_factors, cfg.input, cfg.output, reduce_by=["word1", "word2"])


if __name__ == "__main__":
    main()
