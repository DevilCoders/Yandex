import numpy as np
import pylev
import typing


class CachingLevenshteinEngine:
    def __init__(self):
        self.cache = {}

    def __call__(self, hypothesis, reference):
        current_hash = str(hypothesis) + '_' + str(reference)
        if current_hash in self.cache:
            return self.cache[current_hash]

        result = float(pylev.levenshtein(hypothesis, reference)) / max(len(hypothesis), len(reference), 1)
        self.cache[current_hash] = result
        return result
