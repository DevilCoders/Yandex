# coding=utf8
import liblemmer_python_binding
from collections import defaultdict
import codecs
import pytest


class WordStatsCalcer:
    def __init__(self, langs):
        self.langs = langs

    def __call__(self, word):
        bastards = 0
        forms = 0
        foundlings = 0
        bastard_forms = 0
        analyze = liblemmer_python_binding.AnalyzeWord(word, self.langs)
        lemmas = len(analyze)
        for lemma in analyze:
            if lemma.Bastardness & 0x8:
                foundlings += 1
                continue
            current_forms = len(lemma.Generate())
            if lemma.Bastardness & 0x1:
                bastards += 1
                bastard_forms += current_forms
            forms += current_forms
        return {
            'lemmas': lemmas,
            'bastards': bastards,
            'forms': forms,
            'bastard forms': bastard_forms,
            'foundlings': foundlings,
        }


words = defaultdict(list)
for line in codecs.open('texts', encoding='utf8'):
    fields = line.rstrip(u'\n').split('\t', 1)
    words[fields[0]].append(fields[1])


@pytest.mark.parametrize("lang", ['arm', 'blr', 'bul', 'cze', 'eng', 'fre', 'ger', 'ind', 'ita', 'kaz', 'pol', 'por', 'rum', 'rus', 'spa', 'tat', 'tur', 'ukr', ])
def test_stats(lang):
    stats = list(map(WordStatsCalcer([lang]), words[lang]))
    result = {key: sum(x[key] for x in stats) for key in ['lemmas', 'bastards', 'forms', 'bastard forms', 'foundlings']}
    result['words'] = len(words[lang])
    return result
