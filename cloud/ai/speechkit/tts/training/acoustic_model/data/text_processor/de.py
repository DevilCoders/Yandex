import json
import math
import random
from typing import List, Tuple
import re
import sys
from .base import TextProcessorBase


class TextProcessorDe(TextProcessorBase):
    def __init__(self, config: dict):
        self.punctuation = config["punctuation"]
        self.phones = config["phones"]
        self.sil_tags = ["SIL1", "SIL2", "SIL3", "SIL4", "SIL5"]
        self.special = config["special"]
        self.substitutions = config["substitutions"]
        self.sil_duration_bins = [4, 8, 16, 24, 32, float("inf")]
        self.symbols = self.punctuation + self.phones + self.sil_tags + self.special
        self.pad_id = 0

        self.symbol_to_id = {s: i for i, s in enumerate(self.symbols, start=1)}
        self.id_to_symbol = {i: s for i, s in enumerate(self.symbols, start=1)}

    @property
    def vocab_size(self):
        return len(self.symbols) + 1  # symbols with padding

    @classmethod
    def from_file(cls, config_path):
        with open(config_path) as f:
            config = json.load(f)
        return cls(config)

    def decode_utterance(self, utterance: dict, text: str, accented_text: str) -> Tuple[List[str], List[int], List[int]]:
        symbols = []
        durations = []
        accents = []

        lower_text = text.lower()

        for token in lower_text:
            symbols.append(token)
            # TODO: fix self._align() in sample_builder
            durations.append(20)
            accents.append(0)

        if symbols[0] != "BOS":
            symbols = ["BOS"] + symbols
            durations = [0] + durations
            accents = [0] + accents
        if symbols[-1] != "EOS":
            symbols += ["EOS"]
            durations += [0]
            accents += [0]

        return symbols, durations, accents

    def encode_symbols(self, symbols: List[str]) -> List[int]:
        return [self.symbol_to_id[s] for s in symbols]

    def decode_ids(self, ids: List[int]) -> List[str]:
        return [self.id_to_symbol[i] for i in ids]
