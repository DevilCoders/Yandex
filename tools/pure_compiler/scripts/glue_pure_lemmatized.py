#!/usr/bin/env python
#-*- coding: utf-8 -*-

import sys

LM_MARK = "#%"


def iterate_glued_lines(inp):
    has_lang = set()
    keep_lang = set("mis")
    prev_info = tuple(["_lemma_", "_lang_", "_flag_"])
    prev_num = 0

    for line in inp:
        lemma, lang, flag, num = line.split()
        num = int(num)
        is_lemma = lemma.startswith(LM_MARK)
        if is_lemma:
            lemma = lemma[len(LM_MARK):]
        is_form = int(flag) != 0

        if (
            is_lemma or
            is_form or
            lang in keep_lang or
            lang not in has_lang
        ):
            curr_info = tuple([lemma, lang, flag])
            if curr_info == prev_info:
                prev_num += num
            else:
                if prev_num > 0:
                    yield "{prev[0]}\t{prev[1]}\t{prev[2]}\t{num}".format(prev=prev_info, num=prev_num)
                prev_info = curr_info
                prev_num = num
                if is_lemma:
                   has_lang.add(lang)

    if prev_num > 0:
        yield "{prev[0]}\t{prev[1]}\t{prev[2]}\t{num}".format(prev=prev_info, num=prev_num)


def main():
    with sys.stdin as inp:
        for line in iterate_glued_lines(inp):
            print line


if __name__ == "__main__":
    main()
