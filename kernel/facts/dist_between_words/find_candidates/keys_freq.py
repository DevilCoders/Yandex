#!/usr/bin/env python
# -*- encoding:utf-8 -*-

from __future__ import print_function
import argparse
import yt.wrapper as yt


def getArgs():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-i', '--input', help='Input table with key column', required=True)
    parser.add_argument('--output', help='Output MR directory', required=True)
    return parser.parse_args()


def build_count(key, vals):
    key = key["key"]
    total = 0
    for rec in vals:
        total += 1
    yield {
        'key': key,
        'count': total
    }


def main():
    cfg = getArgs()
    yt.run_reduce(build_count, cfg.input, cfg.output, reduce_by="key")
    yt.run_sort(cfg.output, cfg.output, sort_by="count")


if __name__ == "__main__":
    main()
