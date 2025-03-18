#!/usr/bin/env python
#-*- coding: utf-8 -*-

import sys
from subprocess import Popen, PIPE, check_output

LMTEST = "./lemmer-test"
LMTEST_FP = [LMTEST, "--fingerprints"]
LMTEST_LM = [LMTEST, "-c", "-m"]  # don't forget to add the language mark

LANGS = {
    "0": "mis",
    "1": "ru",
    "2": "en",
    "3": "pl",
    "4": "hu",
    "5": "uk",
    "6": "de",
    "7": "fr",
    "8": "tt",
    "9": "be",
    "10": "kk",
    "11": "sq",
    "12": "es",
    "13": "it",
    "14": "hy",
    "15": "da",
    "16": "pt",
    "17": "mis",
    "18": "sk",
    "19": "sl",
    "20": "nl",
    "21": "bg",
    "22": "ca",
    "23": "hr",
    "24": "cs",
    "25": "el",
    "26": "he",
    "27": "no",
    "28": "mk",
    "29": "sv",
    "30": "ko",
    "31": "la",
    "32": "bas-ru",
    "33": "mis",
    "34": "mis",
    "35": "mis",
    "36": "mis",
    "37": "mis",
    "38": "mis",
    "39": "fi",
    "40": "et",
    "41": "lv",
    "42": "lt",
    "43": "ba",
    "44": "tr",
    "45": "ro",
    "46": "mn",
    "47": "uz",
    "48": "ky",
    "49": "tg",
    "50": "tk",
    "51": "sr",
    "52": "az",
    "53": "bas-en",
    "54": "ka",
    "55": "ar",
    "56": "fa",
    "57": "cu",
}


def get_languages_to_lemmatize():
    output = check_output(LMTEST_FP)

    fingerprints = {}
    for line in output.splitlines():
        if not line.startswith(" "):
            continue
        line = line.strip()
        fields = line.split(":")
        if len(fields) == 2:
            lang, fp = fields
            fingerprints[lang] = fp.strip()

    fingerprints.pop("mis", None)  # unknown language

    print >>sys.stderr, "Will lemmatize the following languages:", " ".join(sorted(fingerprints.iterkeys()))
    return set(fingerprints.iterkeys())


class LemmerTestWrapper(object):

    """Class wraps ./lemmer-test binary

    Can be used as contextmanager. Creates one pipe with lemmer-test per language.
    """

    def __init__(self, languages):
        """languages - set of accepted languages"""
        self.languages = languages
        self.lemmers = {}

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        for lemmer in self.lemmers.itervalues():
            lemmer.communicate()

    def get_lemmer(self, lang):
        """Returns pipe with lemmer for lang.

        Creating pipe on first request. If lang is unsupported None is returned.
        """
        if lang not in self.languages:
            return None
        if lang not in self.lemmers:
            cmd = LMTEST_LM + [lang]
            self.lemmers[lang] = Popen(cmd, stdin=PIPE, stdout=PIPE)
        return self.lemmers[lang]

    def analyse_word(self, word, lang):

        """Returns analysis as list of (lang, lemma) tuples"""

        analysis = []

        lemmer = self.get_lemmer(lang)
        if not lemmer:
            return analysis

        # setting input
        print >>lemmer.stdin, word

        # parsing output
        unique = set()
        while True:
            res = lemmer.stdout.readline()
            res = res.strip()
            if not res:
                break
            lemma, rule_id, qual, gr_stem, gr_flex, form, mt1, mt2, lg, flags = res.split(" ")
            if lg != lang:
                continue
            if word != form:
                continue
            if "dist" in gr_stem:
                continue
            if "dist" in gr_flex:
                continue
            key = tuple([lg, lemma])
            if key in unique:
                continue
            unique.add(key)
            analysis.append(key)

        return analysis


def main():
    languages = get_languages_to_lemmatize()

    with LemmerTestWrapper(languages) as lemmer, sys.stdin as inp:
        for line in inp:
            line = line.strip()
            form, lang, flag, num = line.split()

            # compatibility with lemmatize_pure.pl, unneeded?
            form = "".join(ch for ch in form if (ord(ch) >= 32 or ord(ch) == 1))
            lang = LANGS.get(lang, lang)

            print "{f}\t{lg}\t{fl}\t{num}".format(f=form, lg=lang, fl=flag, num=num)

            # if line is not form, skip
            if not (int(flag) & 2):
                continue

            analysis = lemmer.analyse_word(form, lang)
            for lg, lemma in analysis:
                # is it correct to mix tabs and spaces?
                print "#%{lemma} {lang} 0 {num}".format(lemma=lemma, lang=lg, num=num)


if __name__ == "__main__":
    main()
