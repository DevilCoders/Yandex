#!/usr/bin/env python
#-*- coding: utf-8 -*-

import sys


def iterate_collapsed_lines(inp):
    prev_info = tuple(["_lemma_", "_lang_", "_flag_"])
    max_num = 0
    for line in inp:
        lemma, lang, flag, num = line.split()
        num = int(num)
        curr_info = tuple([lemma, lang, flag])
        if curr_info != prev_info:
            if max_num > 0:
                yield "{prev[0]}\t{prev[1]}\t{prev[2]}\t{num}".format(prev=prev_info, num=max_num)
            prev_info = curr_info
            max_num = num
        max_num = max(num, max_num)
    if max_num > 0:
        yield "{prev[0]}\t{prev[1]}\t{prev[2]}\t{num}".format(prev=prev_info, num=max_num)


def main():
    with sys.stdin as inp:
        for line in iterate_collapsed_lines(inp):
            print line


if __name__ == "__main__":
    main()
