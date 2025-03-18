import yatest.common
import sys
import subprocess
import itertools
from collections import defaultdict
import pytest


def get_lemmas(text, lang, disamb):
    options = [yatest.common.binary_path("tools/lemmer-test/lemmer-test"), "-c"]
    if lang is not None:
        options.extend(["-m", lang])
    if disamb is not None:
        options.extend(["--disamb", disamb])
    proc = subprocess.Popen(
        options,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
    )
    lemmer_result = proc.communicate(text)[0].split('\n')
    lemmas = []
    for key, lines in itertools.groupby(lemmer_result, bool):
        if key:
            cur_lemmas = []
            for line in lines:
                fields = line.split(' ')
                if disamb is not None:
                    weight = float(fields[-1])
                else:
                    weight = None
                cur_lemmas.append((fields[0], weight))
            lemmas.append(cur_lemmas)
    return lemmas


def get_stats(texts, lang, disamb):
    lemmas = get_lemmas('\n'.join(texts), lang, disamb)
    stats = {}
    stats["texts"] = len(texts)
    stats["words"] = len(lemmas)
    stats["lemmas"] = sum(map(len, lemmas))
    if disamb is not None:
        stats["good_lemmas"] = sum(w > 0.5 for lemma in lemmas for _, w in lemma)
    return stats


texts = defaultdict(list)
for line in open('texts.txt'):
    fields = line.rstrip('\n').split('\t', 1)
    texts[fields[0]].append(fields[1])


@pytest.mark.parametrize(
    "lang",
    [
        "ara",
        "arm",
        "bel",
        "bul",
        "cze",
        "eng",
        "fre",
        "ger",
        "ita",
        "kaz",
        "pol",
        "por",
        "rum",
        "rus",
        "spa",
        "tat",
        "tur",
        "ukr",
    ]
)
def test_no_disamb_stat(lang):
    return get_stats(texts[lang], lang, None)


@pytest.mark.parametrize(
    "lang, disamb",
    [
        ("rus", "zel"),
        ("rus", "mx"),
    ]
)
def test_disamb_stat(lang, disamb):
    return get_stats(texts[lang], lang, disamb)
