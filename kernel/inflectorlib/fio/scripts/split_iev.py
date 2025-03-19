#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""Splitting iev and 'ev containing paradigms in two parts"""

import argparse
import codecs
import sys


def iter_lister_chunks(inp):
    line_num = 0
    chunk = []
    for line in inp:
        line_num += 1
        if not line.strip():
            if chunk:
                yield chunk
                chunk = []
            continue
        if line.startswith("@"):
            continue
        chunk.append(line)
    if chunk:
        yield chunk


def write_paradigm(out, lemma, forms):
    out.write(lemma)
    for form in forms:
        out.write(form)
    out.write("\n")


def split_iev(inp, out):
    for chunk in iter_lister_chunks(inp):
        lemma = chunk[0]
        forms = chunk[1:]

        forms_iev = filter(lambda x: u"иев" in x.replace("[", "").replace("]", ""), forms)
        forms_ev = filter(lambda x: u"ьев" in x.replace("[", "").replace("]", ""), forms)

        if len(forms_ev) == len(forms_iev) and len(forms_ev) > 0:
            write_paradigm(out, lemma.replace(u"ьев", u"иев"), forms_iev)
            write_paradigm(out, lemma.replace(u"иев", u"ьев"), forms_ev)
            continue

        write_paradigm(out, lemma, forms)



def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        #formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "-i", "--inp",
        type=lambda s: codecs.open(s, "r", "utf-8"),
        default=codecs.getreader("utf-8")(sys.stdin),
        help="input, stdin by default",
    )
    parser.add_argument(
        "-o", "--out",
        type=lambda s: codecs.open(s, "w", "utf-8"),
        default=codecs.getwriter("utf-8")(sys.stdout),
        help="output, stdout by default",
    )
    args = parser.parse_args()

    with args.inp, args.out:
        split_iev(args.inp, args.out)


if __name__ == "__main__":
    main()

