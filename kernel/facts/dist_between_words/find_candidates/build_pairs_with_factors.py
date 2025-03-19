#!/usr/bin/env python
# -*- encoding:utf-8 -*-

from __future__ import print_function
import argparse
import yt.wrapper as yt
from collections import Counter
import random


def getArgs():
    parser = argparse.ArgumentParser(description='build all pairs of words with base weights')
    parser.add_argument('-i', '--input', help='Input table in format query;norm_query;hosts;urls', required=True)
    parser.add_argument('--output', help='Output MR directory', required=True)
    parser.add_argument('--all-recs', action='store_true', help='Calc on all recs for one key')
    parser.add_argument('--all-keys', action='store_true', help='Calc on all keys (if false only use 10%%)')
    parser.add_argument('--sort', action='store_true', help='Sort input befor reduce')
    parser.add_argument('--max-recs', default=1000, type=int, help='Max number of recs to extract for each key')
    return parser.parse_args()


def build_obj_fact(obj1, obj2, num):
    c1 = Counter(obj1[:num])
    c2 = Counter(obj2[:num])
    diff = sum(filter(lambda v: v > 0, (c1 - c2).itervalues()))
    return 1 - float(diff) / float(max(min(len(obj1), len(obj2), num), 1))


class TReduce(object):
    def __init__(self, all_recs, all_keys, max_recs):
        self.all_recs = all_recs
        self.all_keys = all_keys
        self.max_recs = max_recs

    def __call__(self, key, vals):
        if self.all_keys or abs(hash(key["key"])) % 10 == 0:
            if self.all_recs:
                recs = list(vals)
            else:
                recs = list(vals)
                random.shuffle(recs)
                recs = recs[:self.max_recs]
            for idx, rec in enumerate(recs):
                for rec2 in recs[idx + 1:]:
                    min_rec, max_rec = (rec, rec2) if rec["word"] < rec2["word"] else (rec2, rec)
                    word1 = min_rec["word"]
                    word2 = max_rec["word"]
                    if word1 != word2:
                        host5 = build_obj_fact(min_rec["hosts"], max_rec["hosts"], 5)
                        host10 = build_obj_fact(min_rec["hosts"], max_rec["hosts"], 10)
                        urls5 = build_obj_fact(min_rec["urls"], max_rec["urls"], 5)
                        urls10 = build_obj_fact(min_rec["urls"], max_rec["urls"], 10)
                        yield {
                            "word1": word1,
                            "word2": word2,
                            "host5": host5,
                            "host10": host10,
                            "urls5": urls5,
                            "urls10": urls10,
                            "freq": min(min_rec["freq"], max_rec["freq"])
                        }


def main():
    cfg = getArgs()
    if cfg.sort:
        yt.run_sort(cfg.input, cfg.input, sort_by="key")
    yt.run_reduce(TReduce(cfg.all_recs, cfg.all_keys, cfg.max_recs), cfg.input, cfg.output, reduce_by="key", memory_limit=yt.common.GB * 8)


if __name__ == "__main__":
    main()
