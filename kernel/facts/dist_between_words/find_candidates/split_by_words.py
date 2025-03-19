#!/usr/bin/env python
# -*- encoding:utf-8 -*-

from __future__ import print_function
import argparse
import yt.wrapper as yt
from collections import Counter
import random

def getArgs():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-i', '--input', help='Input table in format query;norm_query;hosts;urls', required=True)
    parser.add_argument('--work-dir', default='//home/search-functionality/antonio/tmp', help='Work MR directory')
    parser.add_argument('--output', help='Output MR directory', required=True)
    return parser.parse_args()


def build_words(rec):
    words = filter(lambda w: not w.startswith("__"), rec["norm_query"].split(" "))
    if len(words) > 1:
        for pos, word in enumerate(words + [""]):
            key = " ".join(words[:pos] + words[pos + 1:])  # or just remove all same words?
            # filter(lambda w: w != word, words)  ?
            yield {
                "key": key,
                "word": word,
                "pos": pos if len(word) > 0 else -1,
                "hosts": rec["hosts"],
                "urls": rec["urls"]
            }


def build_obj_fact(obj1, obj2, num):
    c1 = Counter(obj1[:num])
    c2 = Counter(obj2[:num])
    diff = sum(filter(lambda v: v > 0, (c1 - c2).itervalues()))
    return 1 - float(diff) / float(min(len(obj1), len(obj2), num))


def build_aliases(key, vals):
    key = key["key"]
    # we take only 10% of all data
    if abs(hash(key)) % 10 == 0:
        # take random values - or some reduce tasks take too long time
        recs = random.shuffle(r for r in vals)[:50]
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
                        "urls10": urls10
                    }


def main():
    cfg = getArgs()

    with yt.TempTable(cfg.work_dir) as _map:
       # _map = "//home/search-functionality/antonio/tmp/light20181010_1" # TEMP
        yt.run_map(build_words, cfg.input, _map)
        yt.run_sort(_map, _map, sort_by="key")
        yt.run_reduce(build_aliases, _map, cfg.output, reduce_by="key", memory_limit=yt.common.GB * 4)


if __name__ == "__main__":
    main()
