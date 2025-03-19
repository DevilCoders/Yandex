import json
import math
import random
from typing import List, Tuple
import re
import sys
from .base import TextProcessorBase


class TextProcessorRu(TextProcessorBase):
    def __init__(self, config: dict):
        self.punctuation = config["punctuation"]
        self.phones = config["phones"]
        self.sil_tags = ["SIL1", "SIL2", "SIL3", "SIL4", "SIL5"]
        self.special = config["special"]
        self.substitutions = config["substitutions"]
        self.sil_duration_bins = [4, 8, 16, 24, 32, float("inf")]
        self.symbols = self.punctuation + self.phones + self.sil_tags + self.special
        self.pad_id = 0
        self.sil_tag_prob = 0.3  # TODO: make it a parameter

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

    def to_sil_tag(self, duration_ms: float):
        ms_per_frame = 11.6099773
        num_frames = math.ceil(duration_ms / ms_per_frame)
        for i in range(len(self.sil_duration_bins) - 1):
            if self.sil_duration_bins[i] < num_frames <= self.sil_duration_bins[i + 1]:
                return self.sil_tags[i]

    def _split_utterance_punctuation(self, utterance: dict) -> dict:
        words = []
        for word in utterance["words"]:
            if word["text"] == "yandexttsspecialpauseword":
                words.append(word)
            elif word["phones"][0]["phone"] == "pau":
                duration = word["phones"][0]["duration"] // len(word["text"])
                for token in word["text"]:
                    words.append({
                        "text": token,
                        "phones": [{"duration": duration, "phone": "pau"}]
                    })
            else:
                words.append(word)
        return {"words": words}

    def _setup_accents_in_utterance(self, utterance: dict, accented_text: str):
        def preprocess_text(text: str):
            text = re.sub(r'''([.,;:!?"])''', r" \1 ", text)  # pad punctuation with whitespaces
            text = re.sub(r"\s{2,}", " ", text)  # collapse multiple whitespaces
            text = text.strip().lower()
            return text

        if not accented_text or "**" not in accented_text:
            return
        accented_text = preprocess_text(accented_text)
        for word in re.findall(r"\w* <\[[\w\s]*\]>", accented_text):
            accented_text = accented_text.replace(word, "<[]>")
        accented_text_words = accented_text.split()
        utterance_words = [word for word in utterance["words"] if word["text"] != "yandexttsspecialpauseword"]

        if len(accented_text_words) != len(utterance_words):
            new_list = []
            for word in accented_text_words:
                if word == "**":
                    continue
                if word[:2] == "**" and word[-2:] != "**":
                    new_list.append(f"{word}**")
                else:
                    new_list.append(word)
            accented_text_words = new_list
            if len(accented_text_words) != len(utterance_words):
                sys.stderr.write(f"{accented_text}\n")
                sys.stderr.write(f"{accented_text_words}\n")
                sys.stderr.write(f"{[word['text'] for word in utterance_words]}\n")
                raise AssertionError("number of words in utterance and accented_text do not match")

        SUBSTITUTIONS = [("—", "-")]
        for word, word_meta in zip(accented_text_words, utterance_words):
            if "<[]>" not in word:
                for subst in SUBSTITUTIONS:
                    word = word.replace(*subst)
                    word_meta["text"] = word_meta["text"].replace(*subst)
                if word.replace("**", "") != word_meta["text"]:
                    sys.stderr.write(f"{accented_text}\n")
                    sys.stderr.write(f"{accented_text_words}\n")
                    sys.stderr.write(f"{[word['text'] for word in utterance_words]}\n")
                    raise AssertionError("words in utterance and accented_text do not match")
            if word.startswith("**") and word.endswith("**"):
                word_meta["accent"] = True

    def decode_utterance(self, utterance: dict, text: str, accented_text: str) -> Tuple[List[str], List[int], List[int]]:
        utterance = self._split_utterance_punctuation(utterance)
        self._setup_accents_in_utterance(utterance, accented_text)

        symbols = []
        durations = []
        accents = []

        pause_duration = 0
        for word in utterance["words"]:
            if word["text"] == "yandexttsspecialpauseword":
                pause_duration += word["phones"][0]["duration"]
            elif word["phones"][0]["phone"] == "pau":
                for sub in self.substitutions:
                    word["text"] = word["text"].replace(*sub)
                sym = word["text"][0]
                if sym not in self.punctuation:
                    raise AssertionError("out of vocabulary symbol: " + sym)
                if len(symbols) and symbols[-1] == " ":
                    symbols[-1] = sym
                    assert durations[-1] == 0
                else:
                    symbols += [sym]
                    durations += [0]
                    accents += [0]
                pause_duration += word["phones"][0]["duration"]
            else:
                if not len(symbols):
                    symbols += ["BOS"]
                    durations += [pause_duration]
                    accents += [0]
                else:
                    sig_tag = self.to_sil_tag(pause_duration)
                    if sig_tag is not None and random.random() < self.sil_tag_prob:
                        if symbols[-1] == " ":
                            symbols[-1] = sig_tag
                            durations[-1] += pause_duration
                            symbols += [" "]
                            durations += [0]
                            accents += [0]
                        else:
                            symbols += [self.to_sil_tag(pause_duration), " "]
                            durations += [0, pause_duration]
                            accents += [0, 0]
                    else:
                        if symbols[-1] == " ":
                            durations[-1] += pause_duration
                        else:
                            symbols += [" "]
                            durations += [pause_duration]
                            accents += [0]
                pause_duration = 0
                if len(word["phones"]) == 0:
                    raise AssertionError("empty phone sequence")
                for phone in word["phones"]:
                    if phone["phone"] not in self.phones:
                        raise AssertionError("out of vocabulary phone: " + phone["phone"])
                    symbols += [phone["phone"]]
                    durations += [phone["duration"]]
                    if "accent" in word and word["accent"]:
                        accents += [1]
                    else:
                        accents += [0]

        if pause_duration:
            symbols += ["EOS"]
            durations += [pause_duration]
            accents += [0]

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
