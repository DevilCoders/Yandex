#!/usr/bin/env python
# -*- encoding:utf-8 -*-

from __future__ import print_function
import argparse
import os
import imp
import yt.wrapper as yt


class BasicMR(object):
    def __init__(self):
        self.added_files = {}

    def addFile(self, filepath):
        with open(filepath) as _in:
            filename = os.path.basename(filepath)
            self.added_files[filename] = _in.read()
        return filename

    def persistData(self, filepath):
        if not os.path.exists(filepath):
            with open(filepath, "w") as _out:
                _out.write(self.added_files[filepath])


class Normalizer(BasicMR):
    def __init__(self, normalizer_path, gzt_path, patterns_path, column):
        super(Normalizer, self).__init__()
        self.normalizer_path = self.addFile(normalizer_path)
        self.gzt_path = self.addFile(gzt_path)
        self.patterns_path = self.addFile(patterns_path)
        self.normalizeHandler = None
        self.column = column

    def start(self):
        self.persistData(self.normalizer_path)
        self.persistData(self.gzt_path)
        self.persistData(self.patterns_path)
        normalizelib = imp.load_dynamic('normalizelib', './' + self.normalizer_path)
        self.normalizeHandler = normalizelib.TNormalizer(self.gzt_path, self.patterns_path)

    def __call__(self, rec):
        if "mainResults" in rec:
            query = rec[self.column]
            query_norm = self.normalizeHandler.Normalize(query)
            js = rec["mainResults"]
            hosts = []
            urls = []
            for element in filter(lambda j: j[5] == "web", js):
                hosts.append(element[3])
                urls.append(element[11])

            yield {"query": query,
                   "norm_query": query_norm,
                   "hosts": hosts,
                   "urls": urls}


def getArgs():
    parser = argparse.ArgumentParser(description='Find queries with diff only in one word')
    parser.add_argument('--date', help='date in YYYYMMDD format', required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--normalizer-path", default="normalizelib.so")
    parser.add_argument("--gzt-path", default="special_words.gzt.bin")
    parser.add_argument("--patterns-path", default="all_patterns.txt")
    return parser.parse_args()


def main():
    cfg = getArgs()

    yt.run_map(Normalizer(cfg.normalizer_path, cfg.gzt_path, cfg.patterns_path, "corrected_query"),
               "//home/search-functionality/mt_squeeze/v31/click_props/" + cfg.date,
               cfg.output)


if __name__ == "__main__":
    main()
